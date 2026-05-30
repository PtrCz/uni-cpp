from typing import Type
from .interface import Dataset

from . import (
    case_mapping,
)

type DatasetId = str

def available_datasets() -> dict[DatasetId, Type[Dataset]]:
    datasets: set[Type[Dataset]] = {
        case_mapping.CaseMappingDataset,
    }

    return {dataset.identifier(): dataset for dataset in datasets}
