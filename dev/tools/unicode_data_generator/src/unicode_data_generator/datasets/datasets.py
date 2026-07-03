from typing import Type
from .interface import Dataset, TestDataset

from . import (
    case_mapping,
    composition_mapping,
    utf_encoding,
)

type DatasetId = str
type TestDatasetId = str


def available_datasets() -> dict[DatasetId, Type[Dataset]]:
    datasets: set[Type[Dataset]] = {
        case_mapping.CaseMappingDataset,
        composition_mapping.CompositionMappingDataset,
    }

    return {dataset.identifier(): dataset for dataset in datasets}


def available_test_datasets() -> dict[TestDatasetId, Type[TestDataset]]:
    test_datasets: set[Type[TestDataset]] = {
        case_mapping.CaseMappingTestDataset,
        composition_mapping.CompositionMappingTestDataset,
        utf_encoding.UtfEncodingTestDataset,
    }

    return {test_dataset.identifier(): test_dataset for test_dataset in test_datasets}
