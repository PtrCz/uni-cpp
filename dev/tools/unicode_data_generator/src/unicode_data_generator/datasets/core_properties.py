from typing import Literal, NoReturn
from dataclasses import dataclass
from collections import defaultdict
from collections.abc import Callable

from ..core.test_fail import test_fail
from .interface import Dataset, PrimaryData, EncoderId, ExtraValue, ExtraValues
from ..ucd.code_point_data import CodePoint, CodePointData
from ..core.ranges import usv_range

TRISTATE_VALUES: list[Literal['N', 'M', 'Y']] = ['N', 'Y', 'M']


@dataclass
class Property:
    identifier: str
    value_for_code_point: Callable[[CodePoint, CodePointData], int]
    default_value: int
    bit_position: int
    bit_length: int


properties: dict[str, Property] = {property.identifier: property for property in [
    Property('Lowercase',               lambda code_point, data: int( data[code_point].lowercase                          ), default_value=0, bit_position=0,  bit_length=1),
    Property('Uppercase',               lambda code_point, data: int( data[code_point].uppercase                          ), default_value=0, bit_position=1,  bit_length=1),
    Property('Cased',                   lambda code_point, data: int( data[code_point].cased                              ), default_value=0, bit_position=2,  bit_length=1),
    Property('Case_Ignorable',          lambda code_point, data: int( data[code_point].case_ignorable                     ), default_value=0, bit_position=3,  bit_length=1),
    Property('Alphabetic',              lambda code_point, data: int( data[code_point].alphabetic                         ), default_value=0, bit_position=4,  bit_length=1),
    Property('White_Space',             lambda code_point, data: int( data[code_point].white_space                        ), default_value=0, bit_position=5,  bit_length=1),
    Property('Math',                    lambda code_point, data: int( data[code_point].math                               ), default_value=0, bit_position=6,  bit_length=1),
    Property('Quotation_Mark',          lambda code_point, data: int( data[code_point].quotation_mark                     ), default_value=0, bit_position=7,  bit_length=1),
    Property('Dash',                    lambda code_point, data: int( data[code_point].dash                               ), default_value=0, bit_position=8,  bit_length=1),
    Property('Pattern_Syntax',          lambda code_point, data: int( data[code_point].pattern_syntax                     ), default_value=0, bit_position=9,  bit_length=1),
    Property('Pattern_White_Space',     lambda code_point, data: int( data[code_point].pattern_white_space                ), default_value=0, bit_position=10, bit_length=1),
    Property('ID_Start',                lambda code_point, data: int( data[code_point].id_start                           ), default_value=0, bit_position=11, bit_length=1),
    Property('ID_Continue',             lambda code_point, data: int( data[code_point].id_continue                        ), default_value=0, bit_position=12, bit_length=1),
    Property('XID_Start',               lambda code_point, data: int( data[code_point].xid_start                          ), default_value=0, bit_position=13, bit_length=1),
    Property('XID_Continue',            lambda code_point, data: int( data[code_point].xid_continue                       ), default_value=0, bit_position=14, bit_length=1),
    Property('ID_Compat_Math_Start',    lambda code_point, data: int( data[code_point].id_compat_math_start               ), default_value=0, bit_position=15, bit_length=1),
    Property('ID_Compat_Math_Continue', lambda code_point, data: int( data[code_point].id_compat_math_continue            ), default_value=0, bit_position=16, bit_length=1),
    Property('NFD_Quick_Check',         lambda code_point, data: int( data[code_point].nfd_quick_check                    ), default_value=1, bit_position=17, bit_length=1),
    Property('NFKD_Quick_Check',        lambda code_point, data: int( data[code_point].nfkd_quick_check                   ), default_value=1, bit_position=18, bit_length=1),
    Property('NFC_Quick_Check',         lambda code_point, data: TRISTATE_VALUES.index( data[code_point].nfc_quick_check  ), default_value=2, bit_position=19, bit_length=2),
    Property('NFKC_Quick_Check',        lambda code_point, data: TRISTATE_VALUES.index( data[code_point].nfkc_quick_check ), default_value=2, bit_position=21, bit_length=2),
]}


def _default_encoded_value() -> int:
    value: int = 0

    for property in properties.values():
        value |= (property.default_value << property.bit_position)

    return value


class CorePropertiesDataset(Dataset):
    def __init__(self, data: CodePointData):
        self.code_point_data = data
        self._primary_data = self._generate_primary_data()

    @classmethod
    def identifier(cls) -> str:
        return 'core_properties'


    @classmethod
    def pretty_name(cls) -> str:
        return 'core properties'


    @classmethod
    def necessary_ucd_files(cls) -> set[str]:
        return {
            'ucd/PropList.txt',
            'ucd/DerivedCoreProperties.txt',
            'ucd/DerivedNormalizationProps.txt',
        }
    

    @classmethod
    def optimal_encoder(cls) -> EncoderId:
        return 'multistage_lookup_tables'
    

    def primary_data(self) -> PrimaryData:
        return self._primary_data
    

    def _generate_primary_data(self) -> PrimaryData:
        data: dict[CodePoint, int] = dict()

        for code_point in usv_range():
            encoded_value: int = 0

            for property in properties.values():
                property_value: int = property.value_for_code_point(code_point, self.code_point_data)

                encoded_value |= (property_value << property.bit_position)

            data[code_point] = encoded_value

        return PrimaryData(data, default_value=_default_encoded_value(), mlt_encode_keys_up_to=0x10FFFF)
    

    def extra_values(self) -> ExtraValues:
        _extra_values = {
            f'{property.identifier.lower()}_bit': ExtraValue(f'{property.identifier.lower()}_bit', property.bit_position) for property in properties.values()
        }

        _extra_values['quick_check_no']     = ExtraValue('quick_check_no',      TRISTATE_VALUES.index('N'))
        _extra_values['quick_check_maybe']  = ExtraValue('quick_check_maybe',   TRISTATE_VALUES.index('M'))
        _extra_values['quick_check_yes']    = ExtraValue('quick_check_yes',     TRISTATE_VALUES.index('Y'))

        return ExtraValues(_extra_values)


    def _analyze_impl(self) -> list[str]:
        occurrences = defaultdict(int)

        for code_point in usv_range():
            for ident, property in properties.items():
                value: int = property.value_for_code_point(code_point, self.code_point_data)

                if value != property.default_value:
                    occurrences[ident] += 1

        output: list[str] = []

        for ident in properties.keys():
            output.append(f'number of code points with a non-default value for the {ident} property: {f'{occurrences[ident]:,}'.replace(',', '\'')}')

        return output


    def _test_data_impl(self) -> None | NoReturn:
        for code_point in usv_range():
            for property in properties.values():
                expected: int = property.value_for_code_point(code_point, self.code_point_data)

                try:
                    encoded_value: int = self._primary_data[code_point]
        
                    mask: int = ((1 << property.bit_length) - 1)
                    actual: int = (encoded_value >> property.bit_position) & mask

                    if actual != expected:
                        return test_fail(code_point, expected, actual)

                except Exception:
                    return test_fail(code_point, expected, '<error>')