from typing import Literal
from ..core.optimal_size import optimal_byte_size_for_value


def format_int_as_hex(value: int) -> str:
    match optimal_byte_size_for_value(value, value < 0):
        case 1:
            return f'{value:02X}'
        case 2:
            return f'{value:04X}'
        case 4:
            return f'{value:08X}'
        case 8:
            return f'{value:016X}'


def format_int_as_hex_with_prefix(value: int) -> str:
    match optimal_byte_size_for_value(value, value < 0):
        case 1:
            return f'{value:#04X}'.replace('0X', '0x')
        case 2:
            return f'{value:#06X}'.replace('0X', '0x')
        case 4:
            return f'{value:#010X}'.replace('0X', '0x')
        case 8:
            return f'{value:#018X}'.replace('0X', '0x')


def get_int_type_name(byte_size: Literal[1, 2, 4, 8], is_signed: bool) -> str:
    if is_signed:
        return f'std::int{byte_size * 8}_t'
    else:
        return f'std::uint{byte_size * 8}_t'