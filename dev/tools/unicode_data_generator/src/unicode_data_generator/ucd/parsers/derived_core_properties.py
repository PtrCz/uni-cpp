from ..manager import FileContents
from ..code_point_data import CodePoint, CodePointData
from .simple import SimpleUCDFileParser

class DerivedCorePropertiesParser:
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
                case 'Lowercase':
                    data[code_point].lowercase = True

                case 'Uppercase':
                    data[code_point].uppercase = True

                case 'Cased':
                    data[code_point].cased = True

                case 'Case_Ignorable':
                    data[code_point].case_ignorable = True

                case 'Alphabetic':
                    data[code_point].alphabetic = True

                case 'Math':
                    data[code_point].math = True

                case 'ID_Start':
                    data[code_point].id_start = True

                case 'ID_Continue':
                    data[code_point].id_continue = True

                case 'XID_Start':
                    data[code_point].xid_start = True

                case 'XID_Continue':
                    data[code_point].xid_continue = True

                case _:
                    continue