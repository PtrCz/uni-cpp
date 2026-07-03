from itertools import chain

from ..manager import FileContents
from ..code_point_data import CodePoint, CodePointData
from ...core.ranges import code_point_range, is_precomposed_hangul_syllable, precomposed_hangul_syllable_range
from .utility import is_empty_line, parse_hex, parse_string_field

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

                if len(data_fields[3]) != 0 and data_fields[3] != '0':
                    data[code_point].canonical_combining_class = int(data_fields[3])

                self._update_decomposition_mapping_data(data, code_point, data_fields)

        self._calculate_full_decompositions(data)
                

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


    def _update_decomposition_mapping_data(self, data: CodePointData, code_point: CodePoint, data_fields: list[str]):
        if is_precomposed_hangul_syllable(code_point):
            return self._update_decomposition_mapping_data_for_hangul_syllable(data, code_point)

        decomposition_field: str = data_fields[5]

        if len(decomposition_field.strip()) == 0:
            return

        decomposition_type: str | None = None

        if decomposition_field.startswith('<'):
            tag_end: int = decomposition_field.index('>')

            decomposition_type = decomposition_field[1:tag_end]

            decomposition_field = decomposition_field[tag_end+1:]

        mapping: list[CodePoint] = parse_string_field(decomposition_field)

        data[code_point].decomposition_type = decomposition_type
        data[code_point].decomposition_mapping = mapping


    def _update_decomposition_mapping_data_for_hangul_syllable(self, data: CodePointData, code_point: CodePoint):
        assert is_precomposed_hangul_syllable(code_point)

        s_base = 0xAC00
        l_base = 0x1100
        v_base = 0x1161
        t_base = 0x11A7
        t_count = 28
        n_count = 588

        s_index = code_point - s_base

        match data[code_point].hangul_syllable_type:
            case 'LV':
                l_index = s_index // n_count
                v_index = (s_index % n_count) // t_count
                l_part = l_base + l_index
                v_part = v_base + v_index

                data[code_point].decomposition_mapping = [l_part, v_part]

            case 'LVT':
                lv_index = (s_index // t_count) * t_count
                t_index = s_index % t_count
                lv_part = s_base + lv_index
                t_part = t_base + t_index

                data[code_point].decomposition_mapping = [lv_part, t_part]

            case _:
                raise AssertionError()


    def _calculate_full_decompositions(self, data: CodePointData):

        def calculate_full_canonical_decomposition(code_point: CodePoint) -> list[CodePoint]:
            if code_point not in data:
                return [code_point]

            mapping: list[CodePoint] = data[code_point].decomposition_mapping
            mapping_type: str | None = data[code_point].decomposition_type

            if mapping_type is not None:
                return [code_point]
            
            if mapping == [code_point]:
                return mapping
            
            return list(
                chain.from_iterable(
                    calculate_full_canonical_decomposition(cp)
                    for cp in mapping
                )
            )

        def calculate_full_compatibility_decomposition(code_point: CodePoint) -> list[CodePoint]:
            if code_point not in data:
                return [code_point]
            
            mapping: list[CodePoint] = data[code_point].decomposition_mapping

            if mapping == [code_point]:
                return mapping
            
            return list(
                chain.from_iterable(
                    calculate_full_compatibility_decomposition(cp)
                    for cp in mapping
                )
            )

        for code_point in code_point_range():
            full_canonical_decomposition: list[CodePoint] = calculate_full_canonical_decomposition(code_point)
            full_compatibility_decomposition: list[CodePoint] = calculate_full_compatibility_decomposition(code_point)

            if full_canonical_decomposition != [code_point]:
                data[code_point].full_canonical_decomposition = full_canonical_decomposition

            if full_compatibility_decomposition != [code_point]:
                data[code_point].full_compatibility_decomposition = full_compatibility_decomposition

        self._verify_precomposed_hangul_syllable_decompositions(data)


    def _verify_precomposed_hangul_syllable_decompositions(self, data: CodePointData):
        for code_point in precomposed_hangul_syllable_range():
            assert data[code_point].full_canonical_decomposition == data[code_point].full_compatibility_decomposition

            match data[code_point].hangul_syllable_type:
                case 'LV':
                    assert data[code_point].full_canonical_decomposition == data[code_point].decomposition_mapping

                case 'LVT':
                    decomposition_mapping: list[CodePoint] = data[code_point].decomposition_mapping
                    full_decomposition: list[CodePoint] = data[code_point].full_canonical_decomposition

                    expected_decomposition: list[CodePoint] = []
                    expected_decomposition.extend(data[decomposition_mapping[0]].decomposition_mapping) # decomposition_mapping[0] == LVPart which decomposes into [L, V]
                    expected_decomposition.append(decomposition_mapping[1]) # decomposition_mapping[1] == TPart

                    assert full_decomposition == expected_decomposition

                case _:
                    raise AssertionError()