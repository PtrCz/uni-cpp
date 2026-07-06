from ..datasets.interface import Dataset


def header_guard_macro_for_file(dataset: Dataset) -> str:
    dataset_identifier: str = ''.join(c if c.isalpha() else '_' for c in dataset.identifier().upper())

    return f'UNI_CPP_IMPL_UNICODE_DATA_DATA_{dataset_identifier}_HPP'
    

def namespace_for_dataset(dataset: Dataset) -> str:
    identifier: str = ''.join(c if c.isalpha() else '_' for c in dataset.identifier().lower())

    return f'upp::impl::unicode_data::{identifier}::impl'