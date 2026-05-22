from pathlib import Path
import urllib.request

type FilePath = str
type FileContents = str

class UCDManager:
    unicode_version: str
    cache_dir: Path | None
    _loaded_files: dict[FilePath, FileContents]


    def __init__(self, unicode_version: str, cache_dir: Path | None):
        self.unicode_version = unicode_version
        self._loaded_files = {}

        self.cache_dir = (
            cache_dir / unicode_version / 'UCD'
            if cache_dir is not None
            else None
        )


    def _download_ucd_file(self, filepath: FilePath) -> FileContents:
        file_url: str = (
            f'https://www.unicode.org/Public/{self.unicode_version}/{filepath}'
            if self.unicode_version != 'latest'
            else f'https://www.unicode.org/Public/UCD/latest/{filepath}'
        )

        if self.cache_dir is not None:
            print(f'[*] Downloading file from \'{file_url}\' to \'{self.cache_dir / filepath}\'')
        else:
            print(f'[*] Downloading file from \'{file_url}\'')

        contents: FileContents = urllib.request.urlopen(file_url).read().decode('utf-8')

        if self.cache_dir is not None:
            path: Path = self.cache_dir / filepath

            path.parent.mkdir(parents = True, exist_ok = True)

            with open(path, 'w', encoding='utf-8') as out:
                out.write(contents)

        return contents


    def load_file(self, filepath: FilePath):
        if filepath in self._loaded_files:
            return

        print(f'[*] Loading file: \'{filepath}\'')

        if self.cache_dir is not None:
            path = self.cache_dir / filepath

            if path.exists():
                self._loaded_files[filepath] = path.read_text(encoding='utf-8')
                return
            
        self._loaded_files[filepath] = self._download_ucd_file(filepath)
        

    def get_loaded_files(self) -> dict[FilePath, FileContents]:
        return self._loaded_files