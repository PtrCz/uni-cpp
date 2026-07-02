from ..ucd.code_point_properties import CodePoint

def code_point_range():
    for code_point in range(0x0000, 0x110000):
        yield code_point


def usv_range():
    # From U+0000 to U+D7FF
    for code_point in range(0x0000, 0xD800):
        yield code_point

    # From U+E000 to U+10FFFF
    for code_point in range(0xE000, 0x110000):
        yield code_point


def is_precomposed_hangul_syllable(code_point: CodePoint) -> bool:
    return 0xAC00 <= code_point <= 0xD7A3


def precomposed_hangul_syllable_range():
    for code_point in range(0xAC00, 0xD7A4):
        yield code_point