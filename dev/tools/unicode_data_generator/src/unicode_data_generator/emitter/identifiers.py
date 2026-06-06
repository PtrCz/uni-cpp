from ..datasets.interface import Dataset


def header_guard_macro_for_file(dataset: Dataset, file_ident: str) -> str:
    dataset_identifier: str = ''.join(c if c.isalpha() else '_' for c in dataset.identifier().upper())
    file_identifier: str = ''.join(c if c.isalpha() else '_' for c in file_ident.upper())

    return f'UNI_CPP_IMPL_UNICODE_DATA_DATA_{dataset_identifier}_{file_identifier}_HPP'
    

def namespace_for_dataset(dataset: Dataset) -> str:
    identifier: str = ''.join(c if c.isalpha() else '_' for c in dataset.identifier().lower())

    return f'upp::impl::unicode_data::{identifier}::impl'