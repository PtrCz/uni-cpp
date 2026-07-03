from typing import NoReturn
from collections import defaultdict

from ..core.test_fail import test_fail
from .interface import Dataset, TestDataset, PrimaryData, TestData, ExtraTable, ExtraTables, EncoderId
from ..ucd.code_point_data import CodePoint, CodePointData
from ..core.ranges import usv_range, code_point_range, is_precomposed_hangul_syllable
from ..core import list_utilities

decomposition_types: list[None | str] = \
    [None, 'font', 'noBreak', 'initial', 'medial', 'final', 'isolated', 'circle',
     'super', 'sub', 'vertical', 'wide', 'narrow', 'small', 'square', 'fraction', 'compat']

class DecompositionDataset(Dataset):
    def __init__(self, data: CodePointData):
        self.code_point_data = data

        self.unique_mappings: set[tuple[CodePoint, ...]] = set()

        for code_point, properties in data.items():
            if properties.full_canonical_decomposition != [code_point] and not is_precomposed_hangul_syllable(code_point):
                mapping: list[CodePoint] = properties.full_canonical_decomposition

                self.unique_mappings.add(tuple(mapping))

            if properties.full_compatibility_decomposition != [code_point] and not is_precomposed_hangul_syllable(code_point):
                mapping: list[CodePoint] = properties.full_compatibility_decomposition

                self.unique_mappings.add(tuple(mapping))


        self.mappings_array: list[CodePoint] = list_utilities.fast_superarray(list(list(mapping) for mapping in sorted(self.unique_mappings)))
        
        assert len(self.mappings_array) < 0x4000 # make sure the index will fit into 14 bits

        self._primary_data = self._generate_primary_data()


    @classmethod
    def identifier(cls) -> str:
        return 'decomposition'


    @classmethod
    def pretty_name(cls) -> str:
        return 'decomposition'


    @classmethod
    def necessary_ucd_files(cls) -> set[str]:
        return {
            'ucd/HangulSyllableType.txt',
            'ucd/UnicodeData.txt',
        }
    

    @classmethod
    def optimal_encoder(cls) -> EncoderId:
        return 'multistage_lookup_tables'
    

    def primary_data(self) -> PrimaryData:
        return self._primary_data
    
    
    def _generate_primary_data(self) -> PrimaryData:
        data: dict[CodePoint, int] = dict()

        def calculate_value_for_mapping(code_point: CodePoint, mapping: list[CodePoint]) -> int:
            assert 0 < len(mapping) < 32 # Make sure the length fits into 5 bits. The value zero is reserved for signaling an identity mapping.

            if mapping == [code_point] or is_precomposed_hangul_syllable(code_point):
                return 0

            index: int = list_utilities.find_sublist(self.mappings_array, mapping)

            return (len(mapping) << 14) | index

        for code_point in code_point_range():
            canonical_decomposition = self.code_point_data[code_point].full_canonical_decomposition
            compatibility_decomposition = self.code_point_data[code_point].full_compatibility_decomposition

            decomposition_type_value: int = decomposition_types.index(self.code_point_data[code_point].decomposition_type)

            assert 0 <= decomposition_type_value < 0x20 # Make sure the decomposition_type data fits into 5 bits

            value: int = decomposition_type_value << 38
            value |= calculate_value_for_mapping(code_point, compatibility_decomposition) << 19
            value |= calculate_value_for_mapping(code_point, canonical_decomposition)

            data[code_point] = value

        return PrimaryData(data, default_value=0)
    

    def extra_tables(self) -> ExtraTables:
        return ExtraTables({
            'mappings': ExtraTable('mappings', self.mappings_array),
        })
    

    def _analyze_impl(self) -> list[str]:
        canonical_decomposition_count, compatibility_decomposition_count = 0, 0
        compatibility_decompositions_differing_from_canonical: int = 0

        canonical_decomposition_count_by_length, compatibility_decomposition_count_by_length = defaultdict(int), defaultdict(int)

        decomposition_type_count = defaultdict(int)

        for code_point, properties in self.code_point_data.items():
            if properties.decomposition_mapping != [code_point] and not is_precomposed_hangul_syllable(code_point):
                decomposition_type_count[properties.decomposition_type] += 1

            if properties.full_canonical_decomposition != [code_point] and not is_precomposed_hangul_syllable(code_point):
                canonical_decomposition_count += 1
                canonical_decomposition_count_by_length[len(properties.full_canonical_decomposition)] += 1

            if properties.full_compatibility_decomposition != [code_point] and not is_precomposed_hangul_syllable(code_point):
                compatibility_decomposition_count += 1
                compatibility_decomposition_count_by_length[len(properties.full_compatibility_decomposition)] += 1

                if properties.full_compatibility_decomposition != properties.full_canonical_decomposition:
                    compatibility_decompositions_differing_from_canonical += 1

        output: list[str] = []

        output.append(f'Note: The following statistics exclude the arithmetic hangul syllable decompositions:')
        output.append(f'')
        output.append(f'number of code points with a non-identity full canonical decomposition: {canonical_decomposition_count}')
        output.append(f'number of code points with a non-identity full compatibility decomposition: {compatibility_decomposition_count}')
        output.append(f'number of code points whose full compatibility decomposition differs from their full canonical decomposition: {compatibility_decompositions_differing_from_canonical} ' +
                      f'(including {compatibility_decompositions_differing_from_canonical + canonical_decomposition_count - compatibility_decomposition_count} ' +
                      f'code points with both non-identity canonical and compatibility decompositions)')
        output.append(f'')
        output.append(f'number of non-identity full canonical decompositions by length: {dict(sorted(canonical_decomposition_count_by_length.items()))}')
        output.append(f'number of non-identity full compatibility decompositions by length: {dict(sorted(compatibility_decomposition_count_by_length.items()))}')
        output.append(f'')
        output.append(f'number of code points with a given decomposition type (excluding code points with identity decomposition mappings): ' +
                      f'{dict(sorted(decomposition_type_count.items(), key=lambda item: item[1], reverse=True))}')

        return output


    def _test_decomposition_mapping(self, code_point: CodePoint, encoded_value: int, expected: list[CodePoint]) -> None | NoReturn:
        mapping_index: int = encoded_value & ((1 << 14) - 1)
        mapping_length: int = encoded_value >> 14

        if mapping_length == 0: 
            if expected != [code_point] and not is_precomposed_hangul_syllable(code_point):
                return test_fail(code_point, expected, [code_point])
            else:
                return

        if mapping_length != len(expected):
            return test_fail(code_point, len(expected), mapping_length)

        mapping: list[CodePoint] = []

        for i in range(mapping_length):
            mapping.append(self.mappings_array[mapping_index + i])

        if mapping != expected:
            return test_fail(code_point, expected, mapping)

    def _test_data_impl(self) -> None | NoReturn:
        for code_point in code_point_range():
            try:
                expected_canonical_decomposition: list[CodePoint] = self.code_point_data[code_point].full_canonical_decomposition
                expected_compatibility_decomposition: list[CodePoint] = self.code_point_data[code_point].full_compatibility_decomposition
                expected_decomposition_type: str | None = self.code_point_data[code_point].decomposition_type
                
                value: int = self.primary_data()[code_point]

                canonical_decomposition_value: int = value & ((1 << 19) - 1)
                compatibility_decomposition_value: int = (value >> 19) & ((1 << 19) - 1)

                self._test_decomposition_mapping(code_point, canonical_decomposition_value, expected_canonical_decomposition)
                self._test_decomposition_mapping(code_point, compatibility_decomposition_value, expected_compatibility_decomposition)

                decomposition_type_index: int = value >> 38

                if decomposition_types[decomposition_type_index] != expected_decomposition_type:
                    return test_fail(code_point, expected_decomposition_type, decomposition_types[decomposition_type_index])
                
            except Exception:
                test_fail(code_point, self.code_point_data[code_point].full_canonical_decomposition, '<error>')



class DecompositionTestDataset(TestDataset):
    def __init__(self, code_point_data: CodePointData):
        self.code_point_data = code_point_data

        full_canonical_decomposition_test = TestData('full_canonical_decomposition')
        full_compatibility_decomposition_test = TestData('full_compatibility_decomposition')
        decomposition_type_test = TestData('decomposition_type')

        for code_point in usv_range():
            properties = code_point_data[code_point]

            if self.should_code_point_be_tested(code_point):
                full_canonical_decomposition_test[code_point] = properties.full_canonical_decomposition
                full_compatibility_decomposition_test[code_point] = properties.full_compatibility_decomposition

                decomposition_type_value: int = decomposition_types.index(properties.decomposition_type) if properties.decomposition_type is not None else 0
                decomposition_type_test[code_point] = [decomposition_type_value]


        self._data = [full_canonical_decomposition_test, full_compatibility_decomposition_test, decomposition_type_test]

    def should_code_point_be_tested(self, code_point: CodePoint) -> bool:
        return any((
            self.code_point_data[code_point].full_canonical_decomposition != [code_point],
            self.code_point_data[code_point].full_compatibility_decomposition != [code_point],
            self.code_point_data[code_point].decomposition_type is not None,
            0x10FFF0 < code_point <= 0x10FFFF,
        ))

    @classmethod
    def identifier(cls) -> str:
        return 'decomposition'

    @classmethod
    def pretty_name(cls) -> str:
        return 'decomposition'


    @classmethod
    def necessary_ucd_files(cls) -> set[str]:
        return {
            'ucd/HangulSyllableType.txt',
            'ucd/UnicodeData.txt',
        }
    
    def data(self) -> list[TestData]:
        return self._data
    
    def _test_data_impl(self) -> None | NoReturn:
        data = self.data()
        
        full_canonical_decomposition_test: TestData = data[0]
        full_compatibility_decomposition_test: TestData = data[1]
        decomposition_type_test: TestData = data[2]

        for code_point, decomposition in full_canonical_decomposition_test.data.items():
            if self.code_point_data[code_point].full_canonical_decomposition != decomposition:
                test_fail(code_point, self.code_point_data[code_point].full_canonical_decomposition, decomposition)

        for code_point, compatibility_decomposition in full_compatibility_decomposition_test.data.items():
            if self.code_point_data[code_point].full_compatibility_decomposition != compatibility_decomposition:
                test_fail(code_point, self.code_point_data[code_point].full_compatibility_decomposition, compatibility_decomposition)

        for code_point, decomposition_type in decomposition_type_test.data.items():
            expected: int = decomposition_types.index(self.code_point_data[code_point].decomposition_type)
            actual: int = decomposition_type[0]

            if actual != expected:
                test_fail(code_point, expected, actual)