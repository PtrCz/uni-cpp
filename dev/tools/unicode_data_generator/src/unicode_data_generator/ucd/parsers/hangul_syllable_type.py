from ..manager import FileContents
from ..code_point_data import CodePoint, CodePointData
from .simple import SimpleUCDFileParser

class HangulSyllableTypeParser:
    def __init__(self, contents: FileContents):
        self.contents = contents

    def update_code_point_data(self, data: CodePointData):
        parser = SimpleUCDFileParser(self.contents)

        for code_point_fields in parser.parse():
            if code_point_fields.is_from_at_missing_line:
                continue

            code_point: CodePoint = code_point_fields.code_point
            data_fields: list[str] = code_point_fields.fields

            data[code_point].hangul_syllable_type = data_fields[0]