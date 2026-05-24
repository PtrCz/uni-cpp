from ..manager import FileContents
from ..code_point_data import CodePoint, CodePointData

from .simple import SimpleUCDFileParser
from .utility import parse_hex, parse_string_field

class CaseFoldingParser:
    def __init__(self, contents: FileContents):
        self.contents = contents

    def update_code_point_data(self, data: CodePointData):
        parser = SimpleUCDFileParser(self.contents)

        for code_point_fields in parser.parse():
            code_point: CodePoint = code_point_fields.code_point
            data_fields: list[str] = code_point_fields.fields

            status = data_fields[0]
            mapping = data_fields[1]

            match status:
                case 'C': # common mapping
                    data[code_point].simple_casefold_mapping = parse_hex(mapping)
                    data[code_point].casefold_mapping = [parse_hex(mapping)]

                case 'S': # simple mapping
                    data[code_point].simple_casefold_mapping = parse_hex(mapping)

                case 'F': # full mapping
                    data[code_point].casefold_mapping = parse_string_field(mapping)
