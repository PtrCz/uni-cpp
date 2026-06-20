from abc import ABC, abstractmethod
from collections.abc import Sequence
from dataclasses import dataclass, field
from typing import NoReturn, Literal
from pathlib import Path

from ..ucd.code_point_data import CodePoint, CodePointData
from ..core.optimal_size import optimal_byte_size_for_value
from ..core.tables import Table, Tables

@dataclass
class PrimaryData:
    data: list[int]
    
    def __getitem__(self, index: int):
        return self.data[index]

    def are_values_signed(self) -> bool:
        return any(value < 0 for value in self.data)

    def optimal_value_size(self) -> Literal[1, 2, 4, 8]:
        is_signed: bool = self.are_values_signed()

        return max(optimal_byte_size_for_value(value, is_signed) for value in self.data)


ExtraTable = Table
ExtraTables = Tables


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


type EncoderId = str


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

    @classmethod
    @abstractmethod
    def optimal_encoder(cls) -> EncoderId:
        pass

    @abstractmethod
    def primary_data(self) -> PrimaryData:
        pass

    def extra_tables(self) -> ExtraTables:
        return ExtraTables(dict())
    
    def extra_values(self) -> ExtraValues:
        return ExtraValues(dict())
    
    def analyze(self, output_dir: Path, unicode_version: str):
        print(f'[*] Analyzing {self.pretty_name()} dataset:')
        print()

        analysis: list[str] = self._analyze_impl()

        for line in analysis:
            print(line)

        print()

        filepath: Path = output_dir / unicode_version / 'analysis' / (self.identifier() + '.txt')

        filepath.parent.mkdir(parents=True, exist_ok=True)

        with open(filepath, 'w', encoding='utf-8') as file:
            for line in analysis:
                file.write(line + '\n')

    @abstractmethod
    def _analyze_impl(self) -> list[str]:
        pass

    def test_data(self) -> None | NoReturn:
        print(f'[*] Testing generated {self.pretty_name()} data IR')

        self._test_data_impl()

        print('[+] Test passed')

    @abstractmethod
    def _test_data_impl(self) -> None | NoReturn:
        pass


@dataclass
class TestData:
    identifier: str
    data: dict[CodePoint, list[int]] = field(default_factory=dict)

    def __getitem__(self, key: CodePoint):
        return self.data[key]

    def __setitem__(self, key: CodePoint, value: list[int]):
        self.data[key] = value


class TestDataset(ABC):
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
    def data(self) -> Sequence[TestData]:
        pass

    def test_data(self) -> None | NoReturn:
        print(f'[*] Testing generated {self.pretty_name()} test data')

        self._test_data_impl()

        print('[+] Test passed')

    @abstractmethod
    def _test_data_impl(self) -> None | NoReturn:
        pass