from typing import Type
from .interface import Encoder

from . import (
    multistage_lookup_tables,
)

type EncoderId = str

def available_encoders() -> dict[EncoderId, Type[Encoder]]:
    encoders: set[Type[Encoder]] = {
        multistage_lookup_tables.MultistageLookupTables,
    }

    return {encoder.identifier(): encoder for encoder in encoders}
