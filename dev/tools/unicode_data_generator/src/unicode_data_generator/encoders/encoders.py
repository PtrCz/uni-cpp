from typing import Type
from .interface import Encoder

from . import (
    multistage_lookup_tables,
    minimal_perfect_hash_function,
)

type EncoderId = str

def available_encoders() -> dict[EncoderId, Type[Encoder]]:
    encoders: set[Type[Encoder]] = {
        multistage_lookup_tables.MultistageLookupTables,
        minimal_perfect_hash_function.MinimalPerfectHashFunction,
    }

    return {encoder.identifier(): encoder for encoder in encoders}
