from typing import NoReturn

from .interface import Encoder, EncodedTable, EncodedTables
from ..datasets.datasets import DatasetId
from ..core.test_fail import test_fail
from ..core.progress_bar import ProgressBar
from ..core.optimal_size import optimal_byte_size_for_value

type UnicodeVersion = str
type BlockSize = int

class MultistageLookupTables(Encoder):
    @classmethod
    def identifier(cls) -> str:
        return 'multistage_lookup_tables'
    
    @classmethod
    def pretty_name(cls) -> str:
        return 'multistage lookup tables'
    
    def _generate_encoded_tables(self, use_precomputed_tuning: bool) -> EncodedTables:
        block_size: BlockSize | None = None

        if use_precomputed_tuning:
            if self.unicode_version in precomputed_block_sizes().keys():
                block_size = precomputed_block_sizes()[self.unicode_version][self.dataset.identifier()]

            else:
                print(f'[!] No precomputed block size found for Unicode version {self.unicode_version}')

        if block_size is None:
            print(f'[*] Fine-tuning the block size of {self.dataset.pretty_name()} {self.pretty_name()}')

            block_size = self._fine_tune_block_size()

        print(f'[*] Encoding {self.dataset.pretty_name()} data using multistage lookup tables')

        tables = self._generate(block_size)

        self.block_size = block_size

        self.stage1_needs_extra_lookup: bool = 'stage2_offsets' in tables.tables
        self.stage2_holds_property_values_inplace: bool = 'stage3' not in tables.tables

        return tables
    
    def _fine_tune_block_size(self) -> BlockSize:
        step = 64
        greatest_block_size_initially_checked = 1024

        progress_bar = ProgressBar()
        progress_bar.print_empty()

        # The total number of calls made to `generate_tables` function while fine-tuning.
        # The `greatest_block_size_initially_checked // step` is from the block_sizes definition below and the `15 * 2` is from 2 loop iterations below. 
        total_check_count = 15 * 2 + greatest_block_size_initially_checked // step
        current_check_count = 0

        block_sizes = [n * step for n in range(1, greatest_block_size_initially_checked // step + 1)]

        def total_size_for_given_block_size(block_size: BlockSize) -> int:
            nonlocal current_check_count
            current_check_count += 1

            progress_bar.update(current_check_count / total_check_count)
            
            return self._generate(block_size).total_size()

        # Calculates the total size of tables for each block size in `block_sizes` and returns the `block_size` resulting in the smallest total size
        best_block_size = min(block_sizes, key=total_size_for_given_block_size)

        # Increase the precision with each iteration
        while step >= 8:
            prev_step = step
            step //= 8

            block_sizes = [n * step + best_block_size - prev_step for n in range(1, 16)]

            best_block_size = min(block_sizes, key=total_size_for_given_block_size)

        progress_bar.clear()

        print(f'[+] Most optimal block size found: {best_block_size}')
        return best_block_size
    
    def _generate(self, block_size: BlockSize) -> EncodedTables:
        current_block = []
        blocks = []

        stage1_block_indexes: list[BlockSize] = []

        stage1 = EncodedTable('stage1', [])
        stage2 = EncodedTable('stage2', [])
        stage3 = EncodedTable('stage3', [])

        for value in self.data.data:
            stage3_index = _index_or_append(stage3.values, value)

            current_block.append(stage3_index)

            if len(current_block) == block_size:
                block_index = _index_or_append(blocks, current_block)

                stage1_block_indexes.append(block_index)

                current_block = []

        if len(current_block) != 0:
            blocks.append(current_block)
            block_index = len(blocks) - 1

            stage1_block_indexes.append(block_index)
        
        # overlap the blocks as much as possible to compress the data

        overlapped = _shortest_superarray(blocks)
        stage2.values = overlapped

        for block_index in stage1_block_indexes:
            stage1.values.append(_find_sublist(blocks[block_index], overlapped))

        tables = EncodedTables()

        # check for possible optimizations (see `format` above for the optimizations)

        if stage1.optimal_value_size() > 1:
            unique_stage2_offsets = EncodedTable('stage2_offsets', sorted(set(stage1.values)))

            stage1_current_size = stage1.total_size()

            stage1_new_size = len(stage1.values) * optimal_byte_size_for_value(len(unique_stage2_offsets.values) - 1, is_signed=False)

            stage2_offsets_size = unique_stage2_offsets.total_size()

            saved_bytes = stage1_current_size - stage1_new_size - stage2_offsets_size

            if saved_bytes > 0:
                new_stage1 = EncodedTable('stage1', [])

                for stage2_offset in stage1.values:
                    new_stage1.values.append(unique_stage2_offsets.values.index(stage2_offset))

                tables['stage1'] = new_stage1
                tables['stage2_offsets'] = unique_stage2_offsets

            else:
                tables['stage1'] = stage1

        if stage3.optimal_value_size() <= stage2.optimal_value_size():
            # If the property value type size is smaller or equal to the stage2 index type size,
            # then it's best to place the values inplace (in stage2).

            new_stage2 = EncodedTable('stage2', [])

            for stage3_index in stage2.values:
                new_stage2.values.append(stage3.values[stage3_index])

            tables['stage2'] = new_stage2

        else:
            tables['stage2'] = stage2
            tables['stage3'] = stage3

        return tables


    def _test_data_impl(self) -> None | NoReturn:
        for code_point, property in enumerate(self.data.data):
            try:
                # For the lookup algorithm read `dev/docs/multistage-lookup-tables.md`

                # STAGE 1
                stage1_index: int = code_point // self.block_size
                stage1_value = self._encoded_tables['stage1'].values[stage1_index]

                if self.stage1_needs_extra_lookup:
                    offset: int = self._encoded_tables['stage2_offsets'].values[stage1_value]
                else:
                    offset: int = stage1_value

                # STAGE 2
                stage2_index = offset + code_point % self.block_size
                stage2_value = self._encoded_tables['stage2'].values[stage2_index]

                # STAGE 3
                if self.stage2_holds_property_values_inplace:
                    lookup_result = stage2_value
                else:
                    lookup_result = self._encoded_tables['stage3'].values[stage2_value]
                    
                if lookup_result != property:
                    test_fail(code_point, property, lookup_result)

            except Exception:
                test_fail(code_point, property, '<error>')


def precomputed_block_sizes() -> dict[UnicodeVersion, dict[DatasetId, BlockSize]]:
    return {
        '15.0.0': {
            'case_mapping': 64,
        },
        '15.1.0': {
            'case_mapping': 64,
        },
        '16.0.0': {
            'case_mapping': 64,
        },
        '17.0.0': {
            'case_mapping': 40,
        },
        '18.0.0': {
            'case_mapping': 40,
        },
        'latest': {
            'case_mapping': 40,
        },
    }


def _index_or_append(l: list, value) -> int:
    try:
        return l.index(value)
    
    except ValueError:
        l.append(value)
        return len(l) - 1
    

# from: https://stackoverflow.com/a/17870684
def _find_sublist(sublist: list, list: list) -> int:
    sublist_length = len(sublist)

    for ind in (i for i,e in enumerate(list) if e==sublist[0]):
        if list[ind:ind + sublist_length]==sublist:
            return ind
    
    raise AssertionError()


def _overlap(a: list, b: list) -> int:
    max_overlap = 0
    for i in range(1, min(len(a), len(b)) + 1):
        if a[-i:] == b[:i]:
            max_overlap = i
    return max_overlap


def _merge_arrays(arr1: list, arr2: list) -> list:
    overlap_ab = _overlap(arr1, arr2)
    overlap_ba = _overlap(arr2, arr1)
    if overlap_ab >= overlap_ba:
        return arr1 + arr2[overlap_ab:]
    else:
        return arr2 + arr1[overlap_ba:]


def _shortest_superarray(arrays: list[list]) -> list:
    while len(arrays) > 1:
        max_olap = -1
        best_pair = (0, 1)
        merged = []
        for i in range(len(arrays)):
            for j in range(len(arrays)):
                if i != j:
                    merged_candidate = _merge_arrays(arrays[i], arrays[j])
                    olap = len(arrays[i]) + len(arrays[j]) - len(merged_candidate)
                    if olap > max_olap:
                        max_olap = olap
                        best_pair = (i, j)
                        merged = merged_candidate
        i, j = best_pair
        new_arrays = [arrays[k] for k in range(len(arrays)) if k != i and k != j]
        new_arrays.append(merged)
        arrays = new_arrays
    return arrays[0]