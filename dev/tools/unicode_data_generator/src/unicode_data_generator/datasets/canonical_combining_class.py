from typing import NoReturn

from ..core.test_fail import test_fail
from .interface import Dataset, PrimaryData, EncoderId
from ..ucd.code_point_data import CodePoint, CodePointData
from ..core.ranges import code_point_range, usv_range

class CanonicalCombiningClassDataset(Dataset):
    def __init__(self, data: CodePointData):
        self.code_point_data = data
        self._primary_data = self._generate_primary_data()

    @classmethod
    def identifier(cls) -> str:
        return 'canonical_combining_class'


    @classmethod
    def pretty_name(cls) -> str:
        return 'canonical combining class'


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

        for code_point in usv_range():
            ccc: int = self.code_point_data[code_point].canonical_combining_class

            assert 0 <= ccc < 0xFF

            data[code_point] = ccc

        return PrimaryData(data, default_value=0)
    

    def _analyze_impl(self) -> list[str]:
        reordered_count: int = 0
        not_reordered_count: int = 0

        for code_point in code_point_range():
            if self.code_point_data[code_point].canonical_combining_class == 0:
                not_reordered_count += 1
            else:
                reordered_count += 1

        return [
            f'number of code points with Canonical_Combining_Class == Not_Reordered: {f'{not_reordered_count:,}'.replace(',', '\'')}',
            f'number of code points with Canonical_Combining_Class != Not_Reordered: {f'{reordered_count:,}'.replace(',', '\'')}',
        ]

    def _test_data_impl(self) -> None | NoReturn:
        for code_point in usv_range():
            try:
                expected: int = self.code_point_data[code_point].canonical_combining_class
                
                actual: int = self._primary_data[code_point]

                if actual != expected:
                    return test_fail(code_point, expected, actual)
                
            except Exception:
                test_fail(code_point, self.code_point_data[code_point].canonical_combining_class, '<error>')