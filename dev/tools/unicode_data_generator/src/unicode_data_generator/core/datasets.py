from dataclasses import dataclass

@dataclass(frozen=True)
class Dataset:
    type Name = str

    name: Name
    necessary_ucd_files: frozenset[str]


def datasets() -> dict[Dataset.Name, Dataset]:
    return {
        'case_mapping': Dataset(
            name='case_mapping',
            necessary_ucd_files=frozenset({
                'ucd/UnicodeData.txt',
                'ucd/SpecialCasing.txt',
                'ucd/CaseFolding.txt',
            }),
        ),
    }

def test_datasets() -> dict[Dataset.Name, Dataset]:
    return {
        'utf_encoding': Dataset(
            name='utf_encoding',
            necessary_ucd_files=frozenset(),
        ),
        'case_mapping': Dataset(
            name='case_mapping',
            necessary_ucd_files=frozenset({
                'ucd/UnicodeData.txt',
                'ucd/SpecialCasing.txt',
                'ucd/CaseFolding.txt',
            }),
        ),
    }