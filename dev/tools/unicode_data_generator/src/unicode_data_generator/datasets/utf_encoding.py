from typing import Literal, NoReturn

from ..core.test_fail import test_fail
from .interface import TestDataset, TestData
from ..ucd.code_point_data import CodePoint, CodePointData
from ..core.ranges import usv_range


type UtfEncoding = Literal['UTF-8', 'UTF-16']


def utf_encodings() -> list[UtfEncoding]:
    return ['UTF-8', 'UTF-16']


def _encode_code_point(code_point: CodePoint, utf_encoding: UtfEncoding) -> list[int]:

    if utf_encoding == 'UTF-8':
        return list[int](chr(code_point).encode('utf-8'))

    elif utf_encoding == 'UTF-16':
        encoded_bytes = chr(code_point).encode('utf_16_le')

        return [int.from_bytes(encoded_bytes[i:i+2], 'little') for i in range(0, len(encoded_bytes), 2)]


class UtfEncodingTestData(TestData):
    utf_encoding: UtfEncoding


class UtfEncodingTestDataset(TestDataset):
    def __init__(self, _: CodePointData):
        self._data: list[UtfEncodingTestData] = []

        for utf_encoding in utf_encodings():

            test_data = UtfEncodingTestData(utf_encoding.replace('UTF-', 'utf') + '_encoding')
            test_data.utf_encoding = utf_encoding

            for code_point in usv_range():
                if self.should_code_point_be_tested(code_point):
                    test_data[code_point] = _encode_code_point(code_point, utf_encoding)

            self._data.append(test_data)


    def should_code_point_be_tested(self, code_point: CodePoint) -> bool:
        # test code points near UTF code unit length boundaries

        return any((
            code_point < 0x100,
            0x700 <= code_point <= 0x900,
            0xFF00 <= code_point <= 0x100FF,
            0x10FF00 <= code_point <= 0x10FFFF,
        ))
    

    @classmethod
    def identifier(cls) -> str:
        return 'utf_encoding'

    @classmethod
    def pretty_name(cls) -> str:
        return 'UTF encoding'


    @classmethod
    def necessary_ucd_files(cls) -> set[str]:
        return set()
    
    def data(self) -> list[UtfEncodingTestData]:
        return self._data
    
    def _test_data_impl(self) -> None | NoReturn:
        data = self.data()
        
        for test_data in data:
            for code_point in test_data.data.keys():
                try:
                    expected = _encode_code_point(code_point, test_data.utf_encoding)

                    actual = test_data[code_point]

                    if actual != expected:
                        return test_fail(code_point, expected, actual)
                    
                except Exception:
                    test_fail(code_point, _encode_code_point(code_point, test_data.utf_encoding), '<error>')