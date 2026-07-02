from typing import NoReturn

from .interface import Encoder, EncodedTable, EncodedTables
from ..datasets.interface import PrimaryData
from ..core.internal_error import internal_error
from ..core.test_fail import test_fail

# Based on 'Easy Perfect Minimal Hashing' by Steve Hanov (see https://stevehanov.ca/blog/throw-away-the-keys-easy-minimal-perfect-hashing),
#
# which itself was based on:
#
# Edward A. Fox, Lenwood S. Heath, Qi Fan Chen and Amjad M. Daoud, 
# "Practical minimal perfect hash functions for large databases", CACM, 35(1):105-121

class MinimalPerfectHashFunction(Encoder):
    @classmethod
    def identifier(cls) -> str:
        return 'minimal_perfect_hash_function'
    
    @classmethod
    def pretty_name(cls) -> str:
        return 'minimal perfect hash function'
    
    def _generate_encoded_tables(self, use_precomputed_tuning: bool) -> EncodedTables:
        print(f'[*] Encoding {self.dataset.pretty_name()} data using minimal perfect hash function')

        assert not self.data.are_values_signed()

        self.key_bits = self.data.min_num_of_bits_required_to_store_the_largest_key_with_non_default_value()
        self.value_bits = self.data.min_num_of_bits_required_to_store_the_largest_value()

        max_bits_needed: int = self.key_bits + self.value_bits

        if max_bits_needed > 64:
            internal_error(f'To use the {self.pretty_name()} encoder, the keys with the values of PrimaryData must fit into 64 bits')

        return MinimalPerfectHashFunction._generate(self.data)
    

    @classmethod
    def _generate(cls, data: PrimaryData) -> EncodedTables:
        size: int = len(data.data)

        key_bits: int = data.min_num_of_bits_required_to_store_the_largest_key_with_non_default_value()

        encode_key_value_pair = lambda key, value: (value << key_bits) | key

        # Place all of the keys into buckets

        buckets: list[list[int]] = [[] for i in range(size)]
        intermediate_arr: list[int] = [0] * size
        values_arr: list[int | None] = [None] * size

        for key in data.data.keys():
            if data[key] != data.default_value:
                buckets[hash(key, 0) % size].append(key)

        # Sort the buckets and process the ones with the most items first.

        last_b: int = 0

        buckets.sort(key=len, reverse=True)
        for b in range(size):
            bucket = buckets[b]
            last_b = b

            if len(bucket) <= 1:
                break
            
            d = 1
            item_index = 0
            slots: list[int] = []

            # Repeatedly try different values of d until we find a hash function
            # that places all items in the bucket into free slots

            while item_index < len(bucket):
                slot = hash(bucket[item_index], d) % size

                if values_arr[slot] is not None or slot in slots:
                    d += 1
                    item_index = 0
                    slots = []

                else:
                    slots.append(slot)
                    item_index += 1

            intermediate_arr[hash(bucket[0], 0) % size] = d

            for i in range(len(bucket)):
                values_arr[slots[i]] = encode_key_value_pair(bucket[i], data[bucket[i]])


        # Only buckets with 1 item remain. Process them more quickly by directly
        # placing them into a free slot. Use a negative value of d to indicate
        # this.
        freelist: list[int] = []

        for i in range(size): 
            if values_arr[i] is None:
                freelist.append(i)

        for b in range(last_b, size):
            bucket = buckets[b]

            if len(bucket) == 0:
                break

            slot = freelist.pop()

            # We subtract one to ensure it's negative even if the zero'th slot was used.

            intermediate_arr[hash(bucket[0], 0) % size] = -slot - 1
            values_arr[slot] = encode_key_value_pair(bucket[0], data[bucket[0]])

        values: list[int] = [(value if value is not None else 0) for value in values_arr]

        return EncodedTables({
            'intermediate': EncodedTable('intermediate', intermediate_arr),
            'values': EncodedTable('values', values),
        })


    def _test_data_impl(self) -> None | NoReturn:
        intermediate_arr: list[int] = self.encoded_tables().tables['intermediate'].values
        values_arr: list[int] = self.encoded_tables().tables['values'].values

        for key, value in self.data.data.items():
            try:
                d: int = intermediate_arr[hash(key, 0) % len(intermediate_arr)]

                lookup_result: int = values_arr[-d - 1] if d < 0 else values_arr[hash(key, d) % len(values_arr)]
                
                key_mask: int = (1 << self.key_bits) - 1
                actual_key: int = lookup_result & key_mask

                result: int = (lookup_result >> self.key_bits) if (actual_key == key) else self.data.default_value
                    
                if result != value:
                    test_fail(key, value, result)

            except Exception:
                test_fail(key, value, '<error>')


MASK64 = 0xFFFFFFFFFFFFFFFF

def splitmix64(x: int) -> int:
    x = (x + 0x9E3779B97F4A7C15) & MASK64
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9 & MASK64
    x = (x ^ (x >> 27)) * 0x94D049BB133111EB & MASK64
    x = (x ^ (x >> 31)) & MASK64
    return x

def hash(value: int, d: int) -> int:
    return splitmix64(value ^ splitmix64(d))