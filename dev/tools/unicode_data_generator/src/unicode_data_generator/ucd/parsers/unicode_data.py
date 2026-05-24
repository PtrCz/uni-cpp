from ..manager import FileContents
from ..code_point_data import CodePoint, CodePointData
from .utility import is_empty_line, parse_hex

class UnicodeDataParser:
    def __init__(self, contents: FileContents):
        self.contents = contents

    def update_code_point_data(self, data: CodePointData):
        lines = self.contents.splitlines()

        for i, line in enumerate(lines):
            if is_empty_line(line):
                continue
            
            data_fields = line.split(';')
            listed_code_point = parse_hex(data_fields[0])

            code_points = [listed_code_point] # One line can store data for a range of code points (see https://www.unicode.org/reports/tr44/#Code_Point_Ranges)

            if data_fields[1].startswith('<') and data_fields[1].endswith(', First>'): # code point starting a range
                next_line = lines[i + 1]
                last_code_point_in_range = parse_hex(next_line.split(';')[0])

                code_points.extend(range(listed_code_point + 1, last_code_point_in_range))

            for code_point in code_points:
                self._update_case_mapping_data(data, code_point, data_fields)

                if len(data_fields[2]) != 0:
                    data[code_point].general_category = data_fields[2]
                

    def _update_case_mapping_data(self, data: CodePointData, code_point: CodePoint, data_fields: list[str]):

        simple_uppercase_mapping: CodePoint | None = None

        if len(data_fields[12]) != 0:
            simple_uppercase_mapping = parse_hex(data_fields[12])

            data[code_point].simple_uppercase_mapping = simple_uppercase_mapping
            data[code_point].uppercase_mapping = [simple_uppercase_mapping]

        if len(data_fields[13]) != 0:
            simple_lowercase_mapping: CodePoint = parse_hex(data_fields[13])

            data[code_point].simple_lowercase_mapping = simple_lowercase_mapping
            data[code_point].lowercase_mapping = [simple_lowercase_mapping]

        if len(data_fields[14]) != 0 or simple_uppercase_mapping is not None:
            simple_titlecase_mapping: CodePoint = (
                parse_hex(data_fields[14])
                if len(data_fields[14]) != 0
                else simple_uppercase_mapping
            ) # type: ignore

            data[code_point].simple_titlecase_mapping = simple_titlecase_mapping
            data[code_point].titlecase_mapping = [simple_titlecase_mapping]