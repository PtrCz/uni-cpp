from ..code_point_properties import CodePoint

def is_empty_line(line: str) -> bool:
    return line.strip() == ''

def parse_hex(hex: str) -> int:
    return int(hex, 16)

def parse_string_field(field: str) -> list[CodePoint]:
    return [parse_hex(code_point) for code_point in field.split()]