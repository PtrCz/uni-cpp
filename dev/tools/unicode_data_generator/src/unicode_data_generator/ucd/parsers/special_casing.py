from ..manager import FileContents
from ..code_point_data import CodePoint, CodePointData

from .simple import SimpleUCDFileParser
from .utility import parse_string_field

class SpecialCasingParser:
    def __init__(self, contents: FileContents):
        self.contents = contents

    def update_code_point_data(self, data: CodePointData):
        parser = SimpleUCDFileParser(self.contents)

        for code_point_fields in parser.parse():
            if code_point_fields.fields[3] != '': # skip context and language dependent mappings
                continue

            code_point: CodePoint = code_point_fields.code_point
            data_fields: list[str] = code_point_fields.fields

            data[code_point].lowercase_mapping = parse_string_field(data_fields[0])
            data[code_point].titlecase_mapping = parse_string_field(data_fields[1])
            data[code_point].uppercase_mapping = parse_string_field(data_fields[2])

            