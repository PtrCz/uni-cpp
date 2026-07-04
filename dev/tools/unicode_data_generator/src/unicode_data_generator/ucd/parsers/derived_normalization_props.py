from ..manager import FileContents
from ..code_point_data import CodePoint, CodePointData
from .simple import SimpleUCDFileParser

class DerivedNormalizationPropsParser:
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
                case 'Full_Composition_Exclusion':
                    data[code_point].full_composition_exclusion = True

                case 'NFD_QC':
                    if data_fields[1] == 'N':
                        data[code_point].nfd_quick_check = False

                case 'NFC_QC':
                    if data_fields[1] == 'N':
                        data[code_point].nfc_quick_check = 'N'
                    elif data_fields[1] == 'M':
                        data[code_point].nfc_quick_check = 'M'

                case 'NFKD_QC':
                    if data_fields[1] == 'N':
                        data[code_point].nfkd_quick_check = False

                case 'NFKC_QC':
                    if data_fields[1] == 'N':
                        data[code_point].nfkc_quick_check = 'N'
                    elif data_fields[1] == 'M':
                        data[code_point].nfkc_quick_check = 'M'

                case _:
                    continue
