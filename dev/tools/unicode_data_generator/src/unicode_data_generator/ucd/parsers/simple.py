from ..manager import FileContents
from ..code_point_data import CodePoint, CodePointData
from .utility import is_empty_line, parse_hex

from dataclasses import dataclass

@dataclass
class CodePointFields:
    code_point: CodePoint
    fields: list[str]
    is_from_at_missing_line: bool

class SimpleUCDFileParser:
    def __init__(self, contents: FileContents):
        self.contents = contents


    def _is_at_missing_line(self, line: str):
        if is_empty_line(line):
            return False

        return line.lstrip() == '# @missing: '


    def _code_point_range_in_field_zero(self, field: str) -> list[CodePoint]:
        if '..' in field:
            # code point range

            code_points: list[CodePoint] = []

            index = field.find('..')

            range_start = parse_hex(field[:index])
            range_end = parse_hex(field[index + 2:])

            code_points.extend(range(range_start, range_end + 1))

            return code_points

        else:
            # single code point

            return [parse_hex(field)]


    def parse(self) -> list[CodePointFields]:
        code_point_fields: list[CodePointFields] = []

        for line in self.contents.splitlines():
            is_at_missing_line: bool = False

            if self._is_at_missing_line(line):
                line = line.removeprefix('# @missing: ')
                is_at_missing_line = True

            if '#' in line:
                idx = line.index('#')
                line = line[:idx]

            if is_empty_line(line):
                continue

            data_fields = [field.strip() for field in line.split(';')]

            for code_point in self._code_point_range_in_field_zero(data_fields[0]):
                fields = CodePointFields(code_point, data_fields[1:], is_at_missing_line)

                code_point_fields.append(fields)

        return code_point_fields