from typing import NoReturn

from ..core.test_fail import test_fail
from .interface import Dataset, TestDataset, PrimaryData, TestData, EncoderId
from ..ucd.code_point_data import CodePoint, CodePointData
from ..core.ranges import usv_range, is_precomposed_hangul_syllable


def calculate_compositions(data: CodePointData, *, exclude_hangul: bool) -> dict[tuple[CodePoint, CodePoint], CodePoint]:
    compositions: dict[tuple[CodePoint, CodePoint], CodePoint] = {}

    for code_point in usv_range():
        decomposition_mapping: list[CodePoint] = data[code_point].decomposition_mapping

        if decomposition_mapping == [code_point]:
            continue

        if exclude_hangul and is_precomposed_hangul_syllable(code_point):
            continue

        if data[code_point].decomposition_type is not None:
            continue

        if data[code_point].full_composition_exclusion:
            continue
        
        assert len(decomposition_mapping) == 2

        compositions[tuple(decomposition_mapping)] = code_point # type: ignore

    return compositions


class CompositionMappingDataset(Dataset):
    def __init__(self, data: CodePointData):
        self.code_point_data = data

        self.compositions = calculate_compositions(data, exclude_hangul=True)

        self._primary_data = self._generate_primary_data()


    @classmethod
    def identifier(cls) -> str:
        return 'composition_mapping'


    @classmethod
    def pretty_name(cls) -> str:
        return 'composition mapping'


    @classmethod
    def necessary_ucd_files(cls) -> set[str]:
        return {
            'ucd/HangulSyllableType.txt',
            'ucd/UnicodeData.txt',
            'ucd/DerivedNormalizationProps.txt',
        }
    

    @classmethod
    def optimal_encoder(cls) -> EncoderId:
        return 'minimal_perfect_hash_function'
    

    def primary_data(self) -> PrimaryData:
        return self._primary_data
    
    
    def _generate_primary_data(self) -> PrimaryData:
        data: dict[int, int] = dict()

        for (code_point1, code_point2), composition_mapping in self.compositions.items():
            key: int = (code_point2 << 21) | code_point1

            data[key] = composition_mapping

        return PrimaryData(data, default_value=0x110_000)
    

    def _analyze_impl(self) -> list[str]:
        candidate_count: int = 0
        excluded_by_full_composition_exclusion: int = 0
        precomposed_hangul_syllable_count: int = 0

        for code_point in usv_range():
            decomposition_mapping: list[CodePoint] = self.code_point_data[code_point].decomposition_mapping

            if decomposition_mapping == [code_point]:
                continue

            if is_precomposed_hangul_syllable(code_point):
                precomposed_hangul_syllable_count += 1
                continue

            if self.code_point_data[code_point].decomposition_type is not None:
                continue

            candidate_count += 1

            if self.code_point_data[code_point].full_composition_exclusion:
                excluded_by_full_composition_exclusion += 1

        output: list[str] = []

        output.append(f'number of precomposed hangul syllables: {f'{precomposed_hangul_syllable_count:,}'.replace(',', '\'')}')
        output.append(f'')
        output.append(f'Note: The following statistics exclude the arithmetic hangul syllable composition mappings:')
        output.append(f'')
        output.append(f'number of composition mapping candidates: {candidate_count}')
        output.append(f'number of composition mappings excluded by Full_Composition_Exclusion=True: {excluded_by_full_composition_exclusion}')
        output.append('')
        output.append(f'number of composition mappings: {candidate_count} - {excluded_by_full_composition_exclusion} = {len(self.compositions)}')

        return output

    def _test_data_impl(self) -> None | NoReturn:
        
        for (code_point1, code_point2), expected_composition in self.compositions.items():
            key: int = (code_point2 << 21) | code_point1

            composition_mapping: CodePoint = self.primary_data()[key]

            if composition_mapping != expected_composition:
                test_fail(key, expected_composition, composition_mapping)



class CompositionMappingTestDataset(TestDataset):
    def __init__(self, code_point_data: CodePointData):
        self.code_point_data = code_point_data
        self.compositions = calculate_compositions(code_point_data, exclude_hangul=False)

        self._data = TestData('composition_mappings')

        for (code_point1, code_point2), composition in self.compositions.items():
            encoded_key: int = (code_point2 << 21) | code_point1

            self._data[encoded_key] = [composition]
    

    @classmethod
    def identifier(cls) -> str:
        return 'composition_mapping'

    @classmethod
    def pretty_name(cls) -> str:
        return 'composition mapping'


    @classmethod
    def necessary_ucd_files(cls) -> set[str]:
        return {
            'ucd/HangulSyllableType.txt',
            'ucd/UnicodeData.txt',
            'ucd/DerivedNormalizationProps.txt',
        }
    
    def data(self) -> list[TestData]:
        return [self._data]
    
    def _test_data_impl(self) -> None | NoReturn:
        _test_data = self.data()[0]
        
        for key, composition in _test_data.data.items():
            try:
                code_point1: CodePoint = key & ((1 << 21) - 1)
                code_point2: CodePoint = key >> 21

                expected = self.compositions[(code_point1, code_point2)]

                if composition[0] != expected:
                    return test_fail(key, expected, composition[0])
                
            except Exception:
                test_fail(key, self.compositions[(key & ((1 << 21) - 1), key >> 21)], '<error>')