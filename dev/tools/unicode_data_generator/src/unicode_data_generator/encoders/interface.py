from abc import ABC, abstractmethod
from typing import NoReturn

from ..datasets.interface import Dataset
from ..core.tables import Table, Tables


EncodedTable = Table
EncodedTables = Tables


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