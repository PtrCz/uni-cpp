from dataclasses import dataclass

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

            general_category='Cn',

            canonical_combining_class=0,

            hangul_syllable_type='Not_Applicable',

            decomposition_mapping=[code_point],
            decomposition_type=None,

            full_canonical_decomposition=[code_point],
            full_compatibility_decomposition=[code_point],

            full_composition_exclusion=False,
        )