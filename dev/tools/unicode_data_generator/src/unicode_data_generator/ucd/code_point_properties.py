from dataclasses import dataclass
from typing import Literal

type CodePoint = int

@dataclass
class CodePointProperties:
    value: CodePoint

    simple_lowercase_mapping: CodePoint
    simple_uppercase_mapping: CodePoint
    simple_titlecase_mapping: CodePoint
    simple_casefold_mapping: CodePoint

    lowercase_mapping: list[CodePoint]
    uppercase_mapping: list[CodePoint]
    titlecase_mapping: list[CodePoint]
    casefold_mapping: list[CodePoint]

    lowercase: bool
    uppercase: bool

    cased: bool
    case_ignorable: bool

    alphabetic: bool
    white_space: bool
    math: bool
    quotation_mark: bool
    dash: bool

    pattern_syntax: bool
    pattern_white_space: bool

    id_start: bool
    id_continue: bool
    xid_start: bool
    xid_continue: bool

    id_compat_math_start: bool
    id_compat_math_continue: bool

    nfd_quick_check: bool
    nfkd_quick_check: bool
    nfc_quick_check: Literal['N', 'M', 'Y']
    nfkc_quick_check: Literal['N', 'M', 'Y']

    general_category: str

    canonical_combining_class: int

    hangul_syllable_type: str

    decomposition_mapping: list[CodePoint]
    decomposition_type: str | None

    full_canonical_decomposition: list[CodePoint]
    full_compatibility_decomposition: list[CodePoint]

    full_composition_exclusion: bool

    @classmethod
    def default_properties_for_code_point(cls, code_point: CodePoint):
        return cls(
            value=code_point,

            simple_lowercase_mapping=code_point,
            simple_uppercase_mapping=code_point,
            simple_titlecase_mapping=code_point,
            simple_casefold_mapping=code_point,

            lowercase_mapping=[code_point],
            uppercase_mapping=[code_point],
            titlecase_mapping=[code_point],
            casefold_mapping=[code_point],

            lowercase=False,
            uppercase=False,

            cased=False,
            case_ignorable=False,

            alphabetic=False,
            white_space=False,
            math=False,
            quotation_mark=False,
            dash=False,

            pattern_syntax=False,
            pattern_white_space=False,

            id_start=False,
            id_continue=False,
            xid_start=False,
            xid_continue=False,

            id_compat_math_start=False,
            id_compat_math_continue=False,

            nfd_quick_check=True,
            nfkd_quick_check=True,
            nfc_quick_check='Y',
            nfkc_quick_check='Y',

            general_category='Cn',

            canonical_combining_class=0,

            hangul_syllable_type='Not_Applicable',

            decomposition_mapping=[code_point],
            decomposition_type=None,

            full_canonical_decomposition=[code_point],
            full_compatibility_decomposition=[code_point],

            full_composition_exclusion=False,
        )