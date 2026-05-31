from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import NoReturn

from ..datasets.interface import Dataset
from ..core.optimal_size import optimal_byte_size_for_value


@dataclass
class EncodedTable:
    name: str
    values: list[int]

    def total_size(self) -> int:
        return len(self.values) * self.optimal_value_size()

    def are_values_signed(self) -> bool:
        return any(value < 0 for value in self.values)

    def optimal_value_size(self) -> int:
        is_signed: bool = self.are_values_signed()

        return max(optimal_byte_size_for_value(value, is_signed) for value in self.values)


@dataclass
class EncodedTables:
    tables: dict[str, EncodedTable] = field(default_factory=dict)

    def __getitem__(self, key: str):
        return self.tables[key]

    def __setitem__(self, key: str, value: EncodedTable):
        self.tables[key] = value

    def total_size(self) -> int:
        return sum(table.total_size() for table in self.tables.values())


class Encoder(ABC):
    def __init__(self, dataset: Dataset, use_precomputed_tuning: bool, unicode_version: str):
        self.dataset = dataset
        self.use_precomputed_tuning = use_precomputed_tuning
        self.unicode_version = unicode_version

        self.data = self.dataset.primary_data()

        self._encoded_tables = self._generate_encoded_tables(use_precomputed_tuning)

    @classmethod
    @abstractmethod
    def identifier(cls) -> str:
        pass

    @classmethod
    @abstractmethod
    def pretty_name(cls) -> str:
        pass

    @abstractmethod
    def _generate_encoded_tables(self, use_precomputed_tuning: bool) -> EncodedTables:
        pass

    def encoded_tables(self) -> EncodedTables:
        return self._encoded_tables

    def test_data(self) -> None | NoReturn:
        print(f'[*] Testing generated {self.dataset.pretty_name()} {self.pretty_name()}')

        self._test_data_impl()

        print('[+] Test passed')

    @abstractmethod
    def _test_data_impl(self) -> None | NoReturn:
        pass