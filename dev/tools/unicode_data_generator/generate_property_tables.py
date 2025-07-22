from typing import NoReturn
import sys
import inspect

from generate_cpp_file import table_to_cpp_src

def get_output_path(unicode_version: str):
    return f'output/{unicode_version}/cpp/unicode_data/data'

def internal_error(msg: str, frame: inspect.FrameInfo) -> NoReturn:
    lines = msg.splitlines()
    if len(lines) != 0:
        lines[0] = 'Error: ' + lines[0]

    print(f'[!]\n[!] Internal error!')
    print(f'[!]     Function: {frame.function}')
    print(f'[!]     File:     {frame.filename}')
    print(f'[!]     Line:     {frame.lineno}')
    print(f'[!]')

    for line in lines:
        print(f'[!] {line}')

    return sys.exit(1)

def generate_case_conversion_property_tables(unicode_version: str, code_point_data: dict, case_data: dict) -> dict:
    print('[*] Generating case conversion lookup data')

    def assert_special_mapping_has_expected_length(special_mapping: list[int]) -> None:
        if len(special_mapping) != 2 and len(special_mapping) != 3:
            internal_error('Special case mapping has an unexpected length!\n'
                           'The special case mapping data storage strategy has to be updated.\n' # see long comment below for the current strategy
                           'This function will not work for new Unicode versions until it is updated.', frame = inspect.stack()[1])
        
    def assert_unsigned_value_fits_into_7_bits(value: int) -> None:
        if value < 0 or value > 0x7F:
            internal_error('Generating case conversion lookup data failed!\n'
                           'Failed to fit the case mapping index into 7-bits.\n'
                           'The case conversion data storage strategy has to be updated.\n' # see `dev/docs/case_conversion_tables.md`
                           'This function will not work for new Unicode versions until it is updated.', frame = inspect.stack()[1])

    # read `dev/docs/case_conversion_tables.md`

    cases = ['lowercase', 'uppercase', 'titlecase'] # Note: do not change the order!

    negated_lowercase_mapping_offsets = {-offset for offset in case_data['lowercase']['simple_mappings']['offsets']}

    uppercase_mapping_offsets = case_data['uppercase']['simple_mappings']['offsets']
    titlecase_mapping_offsets = case_data['titlecase']['simple_mappings']['offsets']

    simple_mapping_offsets_union: list = sorted(negated_lowercase_mapping_offsets.union(uppercase_mapping_offsets, titlecase_mapping_offsets))

    prop_value_list = []

    greatest_code_point_with_mapping = max(
        [case_data[case]['mappings']['greatest_code_point_with_mapping'] for case in cases]
    )

    range_end = greatest_code_point_with_mapping + 1
    for code_point in range(0, range_end):
        property_value: int = 0

        for case_index, case in enumerate(cases):
            mapping = code_point_data[code_point][f'{case}_mapping']

            if len(mapping) != 1: # special mapping
                assert_special_mapping_has_expected_length(mapping)

                index: int = case_data[case]['special_mappings']['unique_mappings'].index(mapping)

                assert_unsigned_value_fits_into_7_bits(index)

                # Set the 8-bit MSB to signify this is a special mapping,
                value: int = (1 << 7) | index

            else: # simple mapping
                offset = mapping[0] - code_point

                if case == 'lowercase':
                    offset = -offset # lowercase uses negated offsets (see `dev/docs/case_conversion_tables.md`)

                value: int = simple_mapping_offsets_union.index(offset)

                assert_unsigned_value_fits_into_7_bits(value)

            property_value |= (value << (8 * case_index))

        prop_value_list.append(property_value)

    cpp_extra_contents: str = table_to_cpp_src(
        table = simple_mapping_offsets_union,
        name = 'simple_mapping_offsets',
        value_type_name = 'std::int32_t'
    )

    for case in cases:

        # Compress the special case mappings by storing them in 64-bit integers instead of three 32-bit integers.
        # Since we know the mappings stored in this table are special AND ONLY special, we also know that the length of the mapping is either 2 or 3 code point long
        # (at least there are no special case mappings longer than 3 code points as of Unicode 16.0.0, the most up-to-date version as of writing this).
        # Usually Unicode code points are stored as 32-bit integers (when not encoded), but actually only 21 bits are necessary to store a single code point.
        # As said before, we know that the mapping MUST BE either 2 or 3 code point long so for the longest mapping we would need to store 3 code points,
        # which is 21 bits * 3 code points = 63 bits. We also need to store the information on the length of the mapping (2 or 3).
        # Since we have one bit left to 64-bits we can use this one bit to encode the length of the mapping.
        # And thus we are able to store the full special case mapping in only 64-bits.
        #
        #
        # 1st Note: The 1 bit encoding the length of the mapping is mapped as follows:
        # - 0: mapping's length is 2
        # - 1: mapping's length is 3
        #
        # When decoding the mapping length we can simply take this bits value and add 2 to it to get the mapping length.
        #
        #
        # 2nd Note: The code points are stored in the following order:
        #
        # |      MSB     | M    21-bits   L | M    21-bits   L | M    21-bits   L |
        # |--------------|------------------|------------------|------------------|
        # | [length-bit] | [3rd code point] | [2nd code point] | [1st code point] |
        # 
        # M - MSB of the code point
        # L - LSB of the code point
        #
        # 3rd Note: If the mapping's length is 2 then the bits of the (non-existent) 3rd code point are all 0.
        #

        compressed_special_case_mappings = []

        for mapping in case_data[case]['special_mappings']['unique_mappings']:
            if len(mapping) == 2:
                compressed: int = (mapping[1] << 21) | mapping[0]

            elif len(mapping) == 3:
                compressed: int = (1 << 63) | (mapping[2] << 42) | (mapping[1] << 21) | mapping[0]

            compressed_special_case_mappings.append(compressed)


        cpp_extra_contents += table_to_cpp_src(
            table = compressed_special_case_mappings,
            name = f'special_{case}_mappings',
            value_type_name = 'std::uint64_t'
        )

    for case in cases:
        greatest_code_point: int = case_data[case]['mappings']['greatest_code_point_with_mapping']

        greatest_code_point_str: str = f'{greatest_code_point:#08X}'.replace('0X', '0x')
        cpp_extra_contents += f'constexpr std::uint32_t greatest_code_point_with_{case}_mapping = {greatest_code_point_str};\n'

    return {
        'cpp_output_filepath': f'{get_output_path(unicode_version)}/data.case_conversion.ixx',
        'unicode_version': unicode_version,
        'prop_value_list': prop_value_list,
        'prop_value_type_size': 4,
        'namespace_name': 'case_conversion',
        'property_type_name': 'std::uint32_t',
        'cpp_extra_contents': cpp_extra_contents,
        'hardcoded_optimal_block_size': 64 # Best block size for Unicode 16.0.0
    }