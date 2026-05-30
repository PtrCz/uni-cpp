from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import NoReturn

from ..ucd.code_point_properties import CodePoint
from ..ucd.code_point_data import CodePointData

from ..core.optimal_size import optimal_byte_size_for_value

@dataclass
class PrimaryData:
    data: list[int]
    
    def are_values_signed(self):
        return any(value < 0 for value in self.data)

    def optimal_value_size(self):
        is_signed: bool = self.are_values_signed()

        return max(optimal_byte_size_for_value(value, is_signed) for value in self.data)


@dataclass
class ExtraTable:
    name: str
    values: list[int]

    def are_values_signed(self):
        return any(value < 0 for value in self.values)

    def optimal_value_size(self):
        is_signed: bool = self.are_values_signed()

        return max(optimal_byte_size_for_value(value, is_signed) for value in self.values)


@dataclass
class ExtraValue:
    name: str
    value: int

    def is_signed(self):
        return self.value < 0

    def optimal_size(self):
        return optimal_byte_size_for_value(self.value, self.is_signed())


class Dataset(ABC):
    @abstractmethod
    def __init__(self, data: CodePointData):
        pass

    @classmethod
    @abstractmethod
    def identifier(cls) -> str:
        pass

    @classmethod
    @abstractmethod
    def pretty_name(cls) -> str:
        pass

    @classmethod
    @abstractmethod
    def necessary_ucd_files(cls) -> set[str]:
        pass

    @abstractmethod
    def primary_data(self) -> PrimaryData:
        pass

    def extra_tables(self) -> list[ExtraTable]:
        return []
    
    def extra_values(self) -> list[ExtraValue]:
        return []
    
    def test_data(self) -> None | NoReturn:
        print(f'[*] Testing generated {self.pretty_name()} data IR')

        self._test_data_impl()

        print('[+] Test passed')

    @abstractmethod
    def _test_data_impl(self) -> None | NoReturn:
        pass