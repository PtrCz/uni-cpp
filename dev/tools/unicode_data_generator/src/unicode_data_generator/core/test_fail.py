from typing import NoReturn
import sys
import inspect

from ..core.optimal_size import optimal_byte_size_for_value

def test_fail(key, expected_value, actual_value, frame: inspect.FrameInfo | None = None) -> NoReturn:
    if frame is None:
        frame = inspect.stack()[1]

    print(f'[!] Test failed!')
    print(f'[!]')
    print(f'[!]     The value for {_format(key)} does not match the expected value: {_format(actual_value)} != {_format(expected_value)}')
    print(f'[!]')
    print(f'[!]     Function: {frame.function}')
    print(f'[!]     File:     {frame.filename}')
    print(f'[!]     Line:     {frame.lineno}')
    print(f'[!]')

    return sys.exit(1)


def _format(value) -> str:
    if type(value) is int:
        byte_size: int = optimal_byte_size_for_value(value, is_signed=value < 0)

        return f'{value:#0{byte_size * 2 + 2}X}'.replace('0X', '0x')

    if type(value) is list[int]:
        return f'[{', '.join(
            f'{num:#0{optimal_byte_size_for_value(num, is_signed=num < 0) * 2 + 2}X}'.replace('0X', '0x')
            for num in value
        )}]'

    else:
        return f'{value}'