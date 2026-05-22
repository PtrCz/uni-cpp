from dataclasses import dataclass
from pathlib import Path
from typing import Literal

@dataclass
class GlobalContext:
    cache_dir: Path | None
    output_dir: Path

@dataclass
class GenerateContext:
    unicode_version: str
    target: Literal['all', 'tables', 'tests']
    dataset: str | None
    use_precomputed_tuning: bool | None

@dataclass
class AnalyzeContext:
    unicode_version: str
    dataset: str

@dataclass
class CleanContext:
    unicode_version: str | None
    target: str

@dataclass
class ListContext:
    target: str