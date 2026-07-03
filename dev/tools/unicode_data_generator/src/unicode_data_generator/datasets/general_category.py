from typing import NoReturn
from collections import defaultdict

from ..core.test_fail import test_fail
from .interface import Dataset, TestDataset, PrimaryData, TestData, EncoderId
from ..ucd.code_point_data import CodePointData
from ..core.ranges import code_point_range, usv_range

general_category_list: list[str] = ['Lu', 'Ll', 'Lt', 'Lm', 'Lo', 'Mn', 'Mc', 'Me', 'Nd', 'Nl', 'No', 'Pc', 'Pd', 'Ps', 'Pe',
                                    'Pi', 'Pf', 'Po', 'Sm', 'Sc', 'Sk', 'So', 'Zs', 'Zl', 'Zp', 'Cc', 'Cf', 'Cs', 'Co', 'Cn']


class GeneralCategoryDataset(Dataset):
    def __init__(self, data: CodePointData):
        self.code_point_data = data
        self._primary_data = self._generate_primary_data()

    @classmethod
    def identifier(cls) -> str:
        return 'general_category'


    @classmethod
    def pretty_name(cls) -> str:
        return 'general category'


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
        data: dict[int, int] = dict()

        # Note: use USV range over code point range to avoid unnecessary encoding of surrogates
        for code_point in usv_range():
            general_category = self.code_point_data[code_point].general_category

            index = general_category_list.index(general_category)

            data[code_point] = index

        return PrimaryData(data, default_value=general_category_list.index('Cn'), mlt_encode_keys_up_to=0x10FFFF)
    

    def _analyze_impl(self) -> list[str]:
        general_category_count = defaultdict(int)

        for code_point in code_point_range():
            general_category_count[self.code_point_data[code_point].general_category] += 1

        return [f'number of code points with a given general category: {dict(sorted(general_category_count.items(), key=lambda item: item[1], reverse=True))}']

    def _test_data_impl(self) -> None | NoReturn:
        for code_point in usv_range():
            try:
                expected: str = self.code_point_data[code_point].general_category
                
                index: int = self._primary_data[code_point]

                actual: str = general_category_list[index]

                if actual != expected:
                    return test_fail(code_point, expected, actual)
                
            except Exception:
                test_fail(code_point, self.code_point_data[code_point].general_category, '<error>')



class GeneralCategoryTestDataset(TestDataset):
    def __init__(self, code_point_data: CodePointData):
        self.code_point_data = code_point_data

        self._data = TestData('general_category')

        already_used_general_categories = defaultdict(int)

        for code_point in usv_range():
            gc: str = code_point_data[code_point].general_category

            if already_used_general_categories[gc] < 25:
                already_used_general_categories[gc] += 1

                gc_index: int = general_category_list.index(gc)
                self._data[code_point] = [gc_index]
    

    @classmethod
    def identifier(cls) -> str:
        return 'general_category'

    @classmethod
    def pretty_name(cls) -> str:
        return 'general category'


    @classmethod
    def necessary_ucd_files(cls) -> set[str]:
        return {
            'ucd/HangulSyllableType.txt',
            'ucd/UnicodeData.txt',
        }
    
    def data(self) -> list[TestData]:
        return [self._data]
    
    def _test_data_impl(self) -> None | NoReturn:
        
        for code_point, fields in self._data.data.items():
            try:
                expected: str = self.code_point_data[code_point].general_category

                gc_index: int = fields[0]
                actual: str = general_category_list[gc_index]
    
                if actual != expected:
                    return test_fail(code_point, expected, actual)
                
            except Exception:
                test_fail(code_point, self.code_point_data[code_point].general_category, '<error>')