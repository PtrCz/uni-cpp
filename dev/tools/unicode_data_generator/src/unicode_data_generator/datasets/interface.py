from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import NoReturn, Literal

from ..ucd.code_point_data import CodePointData
from ..core.optimal_size import optimal_byte_size_for_value

@dataclass
class PrimaryData:
    data: list[int]
    
    def are_values_signed(self) -> bool:
        return any(value < 0 for value in self.data)

    def optimal_value_size(self) -> Literal[1, 2, 4, 8]:
        is_signed: bool = self.are_values_signed()

        return max(optimal_byte_size_for_value(value, is_signed) for value in self.data)


@dataclass
class ExtraTable:
    name: str
    values: list[int]

    def total_size(self) -> int:
        return len(self.values) * self.optimal_value_size()

    def are_values_signed(self) -> bool:
        return any(value < 0 for value in self.values)

    def optimal_value_size(self) -> Literal[1, 2, 4, 8]:
        is_signed: bool = self.are_values_signed()

        return max(optimal_byte_size_for_value(value, is_signed) for value in self.values)


@dataclass
class ExtraTables:
    tables: dict[str, ExtraTable]

    def __getitem__(self, key: str):
        return self.tables[key]

    def __setitem__(self, key: str, value: ExtraTable):
        self.tables[key] = value

    def total_size(self) -> int:
        return sum(table.total_size() for table in self.tables.values())


@dataclass
class ExtraValue:
    name: str
    value: int

    def is_signed(self) -> bool:
        return self.value < 0

    def optimal_size(self) -> Literal[1, 2, 4, 8]:
        return optimal_byte_size_for_value(self.value, self.is_signed())


@dataclass
class ExtraValues:
    values: dict[str, ExtraValue]

    def __getitem__(self, key: str):
        return self.values[key]

    def __setitem__(self, key: str, value: ExtraValue):
        self.values[key] = value


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

    def extra_tables(self) -> ExtraTables:
        return ExtraTables(dict())
    
    def extra_values(self) -> ExtraValues:
        return ExtraValues(dict())
    
    def test_data(self) -> None | NoReturn:
        print(f'[*] Testing generated {self.pretty_name()} data IR')

        self._test_data_impl()

        print('[+] Test passed')

    @abstractmethod
    def _test_data_impl(self) -> None | NoReturn:
        pass