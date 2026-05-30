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