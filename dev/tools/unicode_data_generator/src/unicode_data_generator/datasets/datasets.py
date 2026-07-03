from typing import Type
from .interface import Dataset, TestDataset

from . import (
    canonical_combining_class,
    case_mapping,
    composition_mapping,
    decomposition,
    general_category,
    utf_encoding,
)

type DatasetId = str
type TestDatasetId = str


def available_datasets() -> dict[DatasetId, Type[Dataset]]:
    datasets: set[Type[Dataset]] = {
        canonical_combining_class.CanonicalCombiningClassDataset,
        case_mapping.CaseMappingDataset,
        composition_mapping.CompositionMappingDataset,
        decomposition.DecompositionDataset,
        general_category.GeneralCategoryDataset,
    }

    return {dataset.identifier(): dataset for dataset in datasets}


def available_test_datasets() -> dict[TestDatasetId, Type[TestDataset]]:
    test_datasets: set[Type[TestDataset]] = {
        case_mapping.CaseMappingTestDataset,
        composition_mapping.CompositionMappingTestDataset,
        decomposition.DecompositionTestDataset,
        general_category.GeneralCategoryTestDataset,
        utf_encoding.UtfEncodingTestDataset,
    }

    return {test_dataset.identifier(): test_dataset for test_dataset in test_datasets}
