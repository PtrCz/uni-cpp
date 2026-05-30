from typing import NoReturn
import sys
import inspect
from ..ucd.code_point_properties import CodePoint

def test_fail(code_point: CodePoint, expected_value, actual_value, frame: inspect.FrameInfo | None = None) -> NoReturn:
    if frame is None:
        frame = inspect.stack()[1]

    print(f'[!] Test failed!')
    print(f'[!]')
    print(f'[!]     The value for U+{code_point:06X} does not match the expected value: {actual_value} != {expected_value}')
    print(f'[!]')
    print(f'[!]     Function: {frame.function}')
    print(f'[!]     File:     {frame.filename}')
    print(f'[!]     Line:     {frame.lineno}')
    print(f'[!]')

    return sys.exit(1)