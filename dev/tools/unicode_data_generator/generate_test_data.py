import pathlib

def get_test_data_path(unicode_version: str) -> str:
    return f'output/{unicode_version}/test_data'

def unicode_scalar_values():
    # From U+0000 to U+D7FF
    for code_point in range(0x0000, 0xD800):
        yield code_point

    # From U+E000 to U+10FFFF
    for code_point in range(0xE000, 0x110000):
        yield code_point

"""

Note: test data files have the following format:

<code point>:<value>;[<value>;...]

or in proper regex:

^[0-9A-Fa-f]+:([0-9A-Fa-f]+;)+$

"""

def generate_case_conversion_test_data(unicode_version: str, code_point_data: dict, case_data: dict) -> None:
    print('[*] Generating case conversion test data')

    test_data_path: str = get_test_data_path(unicode_version)

    pathlib.Path(test_data_path).mkdir(parents = True, exist_ok = True)

    # Note: Since here we are using the values we parsed from UCD, if the parsing is incorrect the test can pass even tho the mappings are wrong.

    for case in ['lowercase', 'uppercase', 'titlecase']:

        greatest_code_point_with_mapping = case_data[case]['mappings']['greatest_code_point_with_mapping']

        with open(f'{test_data_path}/{case}_mappings.txt', 'w', encoding='utf-8') as file:
            first = True
            
            for code_point in unicode_scalar_values():

                should_be_tested = any((

                    code_point < 0x500, # test for a few first code points
                    len(code_point_data[code_point][f'{case}_mapping']) != 1, # test for all special mappings (<200)
                    
                    # test for the greatest code point with mapping and a few code points next to it
                    greatest_code_point_with_mapping - 100 <= code_point <= greatest_code_point_with_mapping + 100

                ))

                if should_be_tested:

                    if not first:
                        file.write('\n')

                    file.write(f'{code_point:06X}:')

                    for mapping_code_point in code_point_data[code_point][f'{case}_mapping']:
                        file.write(f'{mapping_code_point:06X};')

                    first = False

def generate_utf_encoding_test_data(unicode_version: str, code_point_data: dict, case_data: dict) -> None:
    print('[*] Generating UTF encoding test data')

    test_data_path: str = get_test_data_path(unicode_version)

    pathlib.Path(test_data_path).mkdir(parents = True, exist_ok = True)

    # Note: This functions uses pythons encode function to encode UTF.
    # Then in C++ we check whether these values are the same as the ones from our encoding functions.

    for encoding in ['utf_8', 'utf_16']:

        with open(f'{test_data_path}/{encoding}_encoding.txt', 'w', encoding='utf-8') as file:
            first = True
            
            for code_point in unicode_scalar_values(): # encode every usv
                    
                should_be_tested = any((
                    # test code points near UTF code point boundaries where the encoded byte length changes

                    code_point < 0x100,
                    0x700 <= code_point <= 0x900,
                    0xFF00 <= code_point <= 0x100FF,
                    0x10FF00 <= code_point <= 0x10FFFF,
                ))

                if should_be_tested:

                    if not first:
                        file.write('\n')

                    file.write(f'{code_point:06X}:')

                    if encoding == 'utf_8':
                        code_units = chr(code_point).encode(encoding)

                    elif encoding == 'utf_16':
                        encoded_bytes = chr(code_point).encode('utf_16_le')

                        code_units = [int.from_bytes(encoded_bytes[i:i+2], 'little') for i in range(0, len(encoded_bytes), 2)]

                    for code_unit in code_units:
                        hex_length: int = 2 if encoding == 'utf_8' else 4

                        file.write(f'{code_unit:0{hex_length}X};')

                    first = False