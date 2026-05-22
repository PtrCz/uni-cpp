from dataclasses import dataclass


@dataclass(frozen=True)
class Dataset:
    name: str
    necessary_ucd_files: frozenset[str]


def datasets() -> set[Dataset]:
    return {
        Dataset(
            name='case_mapping',
            necessary_ucd_files=frozenset({
                'ucd/UnicodeData.txt',
                'ucd/SpecialCasing.txt',
            }),
        ),
    }

def test_datasets() -> set[Dataset]:
    return {
        Dataset(
            name='utf_encoding',
            necessary_ucd_files=frozenset(),
        ),
        Dataset(
            name='case_mapping',
            necessary_ucd_files=frozenset({
                'ucd/UnicodeData.txt',
                'ucd/SpecialCasing.txt',
            }),
        ),
    }