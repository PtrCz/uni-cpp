from ..core.internal_error import internal_error

def optimal_byte_size_for_value(value: int, is_signed: bool):
    if is_signed:
        if -0x80 <= value <= 0x7F:
            return 1
        elif -0x8000 <= value <= 0x7FFF:
            return 2
        elif -0x80000000 <= value <= 0x7FFFFFFF:
            return 4
        elif -0x8000000000000000 <= value <= 0x7FFFFFFFFFFFFFFF:
            return 8
        else:
            internal_error('Signed integer value is too large')
    else:
        if 0 <= value <= 0xFF:
            return 1
        elif value <= 0xFFFF:
            return 2
        elif value <= 0xFFFFFFFF:
            return 4
        elif value <= 0xFFFFFFFFFFFFFFFF:
            return 8
        else:
            internal_error('Unsigned integer value is too large')