from pathlib import Path
import hashlib
import pickle

from .manager import FilePath, FileContents
from .code_point_data import CodePointData
from ..core.internal_error import internal_error

from .parsers import (
    unicode_data,
    special_casing,
    case_folding,
)

class Parser:
    unicode_version: str
    cache_dir: Path | None

    def __init__(self, unicode_version: str, cache_dir: Path | None):
        self.unicode_version = unicode_version
        self.cache_dir = (
            cache_dir / unicode_version / 'parser'
            if cache_dir is not None
            else None
        )

    def _generate_hash_from_filepaths(self, filepaths: set[FilePath]) -> str:
        s = '--'.join(sorted(filepaths))

        bytes = s.encode('utf-8')

        hash = hashlib.md5(bytes).hexdigest()

        return hash

    def _load_from_cache(self, cache_filepath: Path) -> CodePointData:
        print('[*] Loading the selected Unicode data from cache')

        with open(cache_filepath, 'rb') as file:
            return pickle.load(file)

    def _reorder_files(self, files: dict[FilePath, FileContents]) -> list[tuple[FilePath, FileContents]]:
        priority = [
            'ucd/UnicodeData.txt',
            'ucd/SpecialCasing.txt',
            'ucd/CaseFolding.txt',
        ]

        order = {k: i for i, k in enumerate(priority)}

        return sorted(
            files.items(),
            key=lambda file: order[file[0]],
        )

    def _update_data_with_file(self, code_point_data: CodePointData, filepath: FilePath, contents: FileContents):
        print(f'[*] Parsing file: \'{filepath}\'')

        match filepath:
            case 'ucd/UnicodeData.txt':
                unicode_data.UnicodeDataParser(contents).update_code_point_data(code_point_data)

            case 'ucd/SpecialCasing.txt':
                special_casing.SpecialCasingParser(contents).update_code_point_data(code_point_data)

            case 'ucd/CaseFolding.txt':
                case_folding.CaseFoldingParser(contents).update_code_point_data(code_point_data)

            case _:
                internal_error('Failed to parse an unknown UCD file!')

    def has_cached_data_from_files(self, filepaths: set[FilePath]) -> bool:
        if self.cache_dir is None:
            return False
        
        hash = self._generate_hash_from_filepaths(filepaths)

        cache_filepath = self.cache_dir / (hash + '.pkl')

        return cache_filepath.exists()

    def load_data_from_files_from_cache(self, filepaths: set[FilePath]) -> CodePointData:
        assert(self.has_cached_data_from_files(filepaths))

        hash = self._generate_hash_from_filepaths(filepaths)

        return self._load_from_cache(self.cache_dir / (hash + '.pkl')) # type: ignore

    def parse_files(self, files: dict[FilePath, FileContents]) -> CodePointData:
        if len(files) == 0:
            return CodePointData()

        hash = self._generate_hash_from_filepaths(set(files.keys()))

        cache_filename = hash + '.pkl'

        if self.cache_dir is not None and (self.cache_dir / cache_filename).exists():
            return self._load_from_cache(self.cache_dir / cache_filename)

        code_point_data = CodePointData()

        for filepath, contents in self._reorder_files(files):
            self._update_data_with_file(code_point_data, filepath, contents)

        if self.cache_dir is not None:
            filepath = self.cache_dir / cache_filename

            self.cache_dir.mkdir(parents = True, exist_ok = True)

            print('[*] Saving the selected Unicode data to cache')

            with open(filepath, 'wb') as file:
                pickle.dump(code_point_data, file)

        return code_point_data