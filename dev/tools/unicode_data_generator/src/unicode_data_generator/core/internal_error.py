from typing import NoReturn
import sys
import inspect

def internal_error(msg: str, frame: inspect.FrameInfo | None = None) -> NoReturn:
    lines = msg.splitlines()
    if len(lines) != 0:
        lines[0] = 'Error: ' + lines[0]

    if frame is None:
        frame = inspect.stack()[1]

    print(f'[!]\n[!] Internal error!')
    print(f'[!]     Function: {frame.function}')
    print(f'[!]     File:     {frame.filename}')
    print(f'[!]     Line:     {frame.lineno}')
    print(f'[!]')

    for line in lines:
        print(f'[!] {line}')

    return sys.exit(1)