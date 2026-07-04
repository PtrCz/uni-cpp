from ..manager import FileContents
from ..code_point_data import CodePoint, CodePointData
from .simple import SimpleUCDFileParser

class PropListParser:
    def __init__(self, contents: FileContents):
        self.contents = contents

    def update_code_point_data(self, data: CodePointData):
        parser = SimpleUCDFileParser(self.contents)

        for code_point_fields in parser.parse():
            if code_point_fields.is_from_at_missing_line:
                continue

            code_point: CodePoint = code_point_fields.code_point
            data_fields: list[str] = code_point_fields.fields

            match data_fields[0]:
                case 'Dash':
                    data[code_point].dash = True

                case 'ID_Compat_Math_Start':
                    data[code_point].id_compat_math_start = True

                case 'ID_Compat_Math_Continue':
                    data[code_point].id_compat_math_continue = True

                case 'Pattern_Syntax':
                    data[code_point].pattern_syntax = True

                case 'Pattern_White_Space':
                    data[code_point].pattern_white_space = True

                case 'Quotation_Mark':
                    data[code_point].quotation_mark = True

                case 'White_Space':
                    data[code_point].white_space = True

                case _:
                    continue