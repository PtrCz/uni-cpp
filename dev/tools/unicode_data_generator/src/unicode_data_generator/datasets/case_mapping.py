import inspect
from dataclasses import dataclass, field
from typing import Literal, NoReturn
from ..core.internal_error import internal_error
from ..core.test_fail import test_fail
from .interface import Dataset, PrimaryData, ExtraTable, ExtraValue
from ..ucd.code_point_data import CodePoint, CodePointData
from ..core.ranges import code_point_range

type CaseName = Literal['lowercase', 'uppercase', 'titlecase', 'casefold']

@dataclass
class CaseMapping:
    name: CaseName
    data: CodePointData

    def get_full_mapping(self, code_point: CodePoint) -> list[CodePoint]:
        match self.name:
            case 'lowercase':
                return self.data[code_point].lowercase_mapping
            case 'uppercase':
                return self.data[code_point].uppercase_mapping
            case 'titlecase':
                return self.data[code_point].titlecase_mapping
            case 'casefold':
                return self.data[code_point].casefold_mapping
            case _:
                raise AssertionError()


def _cases(data: CodePointData) -> list[CaseMapping]:
    return [
        CaseMapping('lowercase', data),
        CaseMapping('uppercase', data),
        CaseMapping('titlecase', data),
        CaseMapping('casefold', data),
    ]


def _default_offsets() -> set[int]:
    return {0}

@dataclass
class CaseMappingData:
    greatest_code_point_with_mapping: CodePoint = -1
    unique_special_mappings: set[tuple[CodePoint, ...]] = field(default_factory=set)
    offsets: set[int] = field(default_factory=_default_offsets)


class CaseMappingDataset(Dataset):
    def __init__(self, data: CodePointData):
        self.cases = _cases(data)

        self.mapping_data = self._get_mapping_data(data)
        self.code_point_data = data
        
        self._init_data()

        self._primary_data = self._generate_primary_data()
        self._extra_tables = self._generate_extra_tables()
        self._extra_values = self._generate_extra_values()


    @classmethod
    def identifier(cls) -> str:
        return 'case_mapping'


    @classmethod
    def pretty_name(cls) -> str:
        return 'case mapping'


    @classmethod
    def necessary_ucd_files(cls) -> set[str]:
        return {
            'ucd/UnicodeData.txt',
            'ucd/SpecialCasing.txt',
            'ucd/CaseFolding.txt',
        }
    

    def primary_data(self) -> PrimaryData:
        return self._primary_data
    
    
    def extra_tables(self) -> list[ExtraTable]:
        return self._extra_tables
    

    def extra_values(self) -> list[ExtraValue]:
        return self._extra_values


    def _init_data(self):
        self.greatest_code_point_with_mapping = max(self.mapping_data[case.name].greatest_code_point_with_mapping for case in self.cases)

        negated_uppercase_mapping_offsets = {-offset for offset in self.mapping_data['uppercase'].offsets}
        negated_titlecase_mapping_offsets = {-offset for offset in self.mapping_data['titlecase'].offsets}

        self.offsets_union = sorted(set().union(
            self.mapping_data['lowercase'].offsets,
            negated_uppercase_mapping_offsets,
            negated_titlecase_mapping_offsets,
            self.mapping_data['casefold'].offsets,
        ))
        
        self.unique_special_mappings: list[tuple[CodePoint, ...]] = sorted(set().union(
            *(self.mapping_data[case.name].unique_special_mappings for case in self.cases)
        ))


    def _generate_primary_data(self) -> PrimaryData:
        data: list[int] = []

        for code_point in range(0, self.greatest_code_point_with_mapping + 1):
            property_value: int = 0

            for case_index, case in enumerate(self.cases):
                mapping = case.get_full_mapping(code_point)

                if len(mapping) != 1: # special mapping
                    self._assert_special_mapping_has_expected_length(mapping)

                    index: int = self.unique_special_mappings.index(tuple(mapping))

                    self._assert_unsigned_value_fits_into_15_bits(index)

                    # Set the 16-bit MSB to signify this is a special mapping,
                    value: int = (1 << 15) | index

                else: # simple mapping
                    offset = mapping[0] - code_point

                    if case.name in {'uppercase', 'titlecase'}:
                        offset = -offset # uppercase and titlecase mappings use negated offsets

                    value: int = self.offsets_union.index(offset)

                    self._assert_unsigned_value_fits_into_15_bits(value)

                property_value |= (value << (16 * case_index))

            data.append(property_value)

        return PrimaryData(data)
    

    def _generate_extra_tables(self) -> list[ExtraTable]:
        special_mappings: list[int] = []

        for mapping in self.unique_special_mappings:
            special_mappings.append(self._compact_special_case_mapping(mapping))

        self.special_mappings_table = special_mappings

        return [
            ExtraTable('simple_mapping_offsets', self.offsets_union),
            ExtraTable('special_mappings', special_mappings),
        ]
    

    def _generate_extra_values(self) -> list[ExtraValue]:
        return [
            ExtraValue(
                name=f'greatest_code_point_with_{case.name}_mapping',
                value=self.mapping_data[case.name].greatest_code_point_with_mapping
            ) for case in self.cases
        ]


    def lookup(self, code_point: CodePoint, case: CaseMapping, case_index: int) -> list[CodePoint]:
        if code_point > self.mapping_data[case.name].greatest_code_point_with_mapping:
            return [code_point]

        value = self._primary_data.data[code_point]

        bit_offset = 16 * case_index
        case_value = (value >> bit_offset) & 0xFFFF

        index = case_value & 0x7FFF

        if case_value & 0x8000 != 0: # special mapping
            mapping = self.special_mappings_table[index]

            length_bit: int = (mapping & (1 << 63)) >> 63

            if length_bit == 0:
                return [
                     mapping        & 0x1FFFFF,
                    (mapping >> 21) & 0x1FFFFF,
                ]
            else:
                return [
                     mapping        & 0x1FFFFF,
                    (mapping >> 21) & 0x1FFFFF,
                    (mapping >> 42) & 0x1FFFFF,
                ]

        else: # simple mapping
            mapping_offset: int =  self.offsets_union[index]

            if case.name in {'uppercase', 'titlecase'}:
                mapping_offset = -mapping_offset

            return [code_point + mapping_offset]
        

    def _test_data_impl(self) -> None | NoReturn:
        for code_point in code_point_range():
            for case_index, case in enumerate(self.cases):
                try:
                    expected = case.get_full_mapping(code_point)
                    
                    actual = self.lookup(code_point, case, case_index)

                    if actual != expected:
                        return test_fail(code_point, expected, actual)
                    
                except Exception:
                    test_fail(code_point, case.get_full_mapping(code_point), '<error>')


    def _compact_special_case_mapping(self, mapping: tuple[CodePoint, ...]) -> int:
        self._assert_special_mapping_has_expected_length(list(mapping))

        if len(mapping) == 2:
            return (mapping[1] << 21) | mapping[0]

        elif len(mapping) == 3:
            return (1 << 63) | (mapping[2] << 42) | (mapping[1] << 21) | mapping[0]
        
        else:
            raise AssertionError()


    def _get_mapping_data(self, data: CodePointData) -> dict[CaseName, CaseMappingData]:
        cases = _cases(data)

        case_data: dict[CaseName, CaseMappingData] = {case.name: CaseMappingData() for case in cases}
        special_mappings: dict[CaseName, list[list[CodePoint]]] = {case.name: list() for case in cases}

        for code_point in code_point_range():
            for case in cases:
                mapping = case.get_full_mapping(code_point)

                if len(mapping) != 1 or mapping[0] != code_point: # the code point doesn't map to itself
                    case_data[case.name].greatest_code_point_with_mapping = code_point
                    
                    if len(mapping) != 1: # special mapping
                        special_mappings[case.name].append(mapping)
                            
                    else: # simple mapping
                        case_data[case.name].offsets.add(mapping[0] - code_point)

        for case in cases:
            case_data[case.name].unique_special_mappings = {tuple(m) for m in special_mappings[case.name]}

        return case_data


    def _assert_special_mapping_has_expected_length(self, special_mapping: list[CodePoint]):
        if len(special_mapping) not in {2, 3}:
            internal_error('Special case mapping has an unexpected length!\n'
                           'The special case mapping data storage strategy has to be updated.\n' # see `dev/docs/case_conversion_tables.md` for the current strategy
                           'This function will not work for new Unicode versions until it is updated.', frame = inspect.stack()[1])
        

    def _assert_unsigned_value_fits_into_15_bits(self, value: int):
        if value < 0 or value > 0x7FFF:
            internal_error('Generating case conversion lookup data failed!\n'
                           'Failed to fit the case mapping index into 15-bits.\n'
                           'The case conversion data storage strategy has to be updated.\n' # see `dev/docs/case_conversion_tables.md` for the current strategy
                           'This function will not work for new Unicode versions until it is updated.', frame = inspect.stack()[1])