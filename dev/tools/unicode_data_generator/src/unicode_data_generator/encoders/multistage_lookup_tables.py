from typing import NoReturn
from concurrent.futures import ProcessPoolExecutor, as_completed
import os

from .interface import Encoder, EncodedTable, EncodedTables
from ..datasets.interface import PrimaryData
from ..datasets.datasets import DatasetId
from ..core.test_fail import test_fail
from ..core.progress_bar import ProgressBar
from ..core.optimal_size import optimal_byte_size_for_value
from ..core import list_utilities

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

        tables = MultistageLookupTables._generate(self.data, block_size)

        self.block_size = block_size

        self.stage1_needs_extra_lookup: bool = 'stage2_offsets' in tables.tables
        self.stage2_holds_property_values_inplace: bool = 'stage3' not in tables.tables

        return tables
    
    def _fine_tune_block_size(self) -> BlockSize:
        step = 64
        greatest_block_size_initially_checked = 1024

        progress_bar = ProgressBar()
        progress_bar.print_empty()

        # The total number of calls made to `MultistageLookupTables._generate()` function while fine-tuning.
        # The `greatest_block_size_initially_checked // step` is from the block_sizes definition below and the `15 * 2` is from 2 loop iterations below. 
        total_check_count = 15 * 2 + greatest_block_size_initially_checked // step
        completed_checks = 0

        
        def find_best_block_size(block_sizes: list[BlockSize], executor: ProcessPoolExecutor) -> BlockSize:
            nonlocal completed_checks

            futures = [
                executor.submit(
                    MultistageLookupTables._evaluate_block_size,
                    self.data,
                    block_size
                )
                for block_size in block_sizes
            ]

            best_block_size: BlockSize | None = None
            best_total_size: int | None = None

            for future in as_completed(futures):
                block_size, total_size = future.result()

                completed_checks += 1
                progress_bar.update(completed_checks / total_check_count)

                if (best_total_size is None or total_size < best_total_size):
                    best_total_size = total_size
                    best_block_size = block_size

            assert best_block_size is not None

            return best_block_size


        block_sizes = [
            n * step
            for n in range(
                1,
                greatest_block_size_initially_checked // step + 1
            )
        ]

        max_workers: int = max(1, (os.cpu_count() or 1) // 4)

        with ProcessPoolExecutor(max_workers=min(max_workers, 61)) as executor:
            best_block_size = find_best_block_size(block_sizes, executor)

            # Increase the precision with each iteration
            while step >= 8:
                prev_step = step
                step //= 8

                block_sizes = [
                    n * step + best_block_size - prev_step
                    for n in range(1, 16)
                ]

                best_block_size = find_best_block_size(block_sizes, executor)

        progress_bar.clear()

        print(f'[+] Most optimal block size found: {best_block_size}')
        return best_block_size
    

    @classmethod
    def _evaluate_block_size(cls, data: PrimaryData, block_size: BlockSize) -> tuple[BlockSize, int]:
        return (
            block_size,
            cls._generate(data, block_size).total_size()
        )


    @classmethod
    def _generate(cls, data: PrimaryData, block_size: BlockSize) -> EncodedTables:
        current_block = []
        blocks = []

        stage1_block_indexes: list[BlockSize] = []

        stage1 = EncodedTable('stage1', [])
        stage2 = EncodedTable('stage2', [])
        stage3 = EncodedTable('stage3', [])

        assert data.mlt_encode_keys_up_to is None or data.mlt_encode_keys_up_to >= data.max_key_with_non_default_value()

        encode_keys_up_to: int = data.mlt_encode_keys_up_to or data.max_key_with_non_default_value()

        for iter in range(encode_keys_up_to + 1):
            value: int = data[iter]

            stage3_index = list_utilities.index_or_append(stage3.values, value)

            current_block.append(stage3_index)

            if len(current_block) == block_size:
                block_index = list_utilities.index_or_append(blocks, current_block)

                stage1_block_indexes.append(block_index)

                current_block = []

        if len(current_block) != 0:
            blocks.append(current_block)
            block_index = len(blocks) - 1

            stage1_block_indexes.append(block_index)
        
        # overlap the blocks as much as possible to compress the data

        overlapped = list_utilities.shortest_superarray(blocks)
        stage2.values = overlapped

        for block_index in stage1_block_indexes:
            stage1.values.append(list_utilities.find_sublist(overlapped, blocks[block_index]))

        tables = EncodedTables()

        # check for possible optimizations

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
        max_key_with_non_default_value: int = self.data.max_key_with_non_default_value()

        # When mlt_encode_keys_up_to is None, its safe to lookup any key, so we lookup `max_key + 1` to test it.
        # When it's not None, it's only safe to lookup keys up to `mlt_encode_keys_up_to`, so we do exactly that.

        check_keys_up_to: int = self.data.max_key_with_non_default_value() + 1 \
                                if self.data.mlt_encode_keys_up_to is None \
                                else self.data.mlt_encode_keys_up_to

        for key in range(check_keys_up_to + 1):
            expected_value: int = self.data[key]

            try:
                if self.data.mlt_encode_keys_up_to is None and key > max_key_with_non_default_value:
                    if expected_value == self.data.default_value:
                        continue
                    else:
                        test_fail(key, expected_value, self.data.default_value)

                # For the lookup algorithm read `dev/docs/multistage-lookup-tables.md`

                # STAGE 1
                stage1_index: int = key // self.block_size
                stage1_value = self._encoded_tables['stage1'].values[stage1_index]

                if self.stage1_needs_extra_lookup:
                    offset: int = self._encoded_tables['stage2_offsets'].values[stage1_value]
                else:
                    offset: int = stage1_value

                # STAGE 2
                stage2_index = offset + key % self.block_size
                stage2_value = self._encoded_tables['stage2'].values[stage2_index]

                # STAGE 3
                if self.stage2_holds_property_values_inplace:
                    lookup_result = stage2_value
                else:
                    lookup_result = self._encoded_tables['stage3'].values[stage2_value]
                    
                if lookup_result != expected_value:
                    test_fail(key, expected_value, lookup_result)

            except Exception:
                test_fail(key, expected_value, '<error>')


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