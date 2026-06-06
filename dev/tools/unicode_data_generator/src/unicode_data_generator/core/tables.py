from dataclasses import dataclass, field
from typing import Literal

from .optimal_size import optimal_byte_size_for_value

@dataclass
class Table:
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
class Tables:
    tables: dict[str, Table] = field(default_factory=dict)

    def __getitem__(self, key: str):
        return self.tables[key]

    def __setitem__(self, key: str, value: Table):
        self.tables[key] = value

    def total_size(self) -> int:
        return sum(table.total_size() for table in self.tables.values())