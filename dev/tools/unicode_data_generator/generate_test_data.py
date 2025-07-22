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

def generate_case_conversion_test_data(unicode_version: str, code_point_data: dict, case_data: dict) -> None:
    print('[*] Generating case conversion test data')

    test_data_path: str = get_test_data_path(unicode_version)

    pathlib.Path(test_data_path).mkdir(parents = True, exist_ok = True)

    # Note: Since here we are using the values we parsed from UCD, if the parsing is incorrect the test can pass even tho the mappings are wrong.

    for case in ['lowercase', 'uppercase', 'titlecase']:

        with open(f'{test_data_path}/{case}_mappings.txt', 'w', encoding='utf-8') as file:
            first = True
            for code_point in unicode_scalar_values():

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

    # Note: This functions uses pythons encode function to encode every USV.
    # Then in C++ we check whether these values are the same as the ones from our encoding functions.

    for encoding in ['utf_8', 'utf_16']:

        with open(f'{test_data_path}/{encoding}_encoding.txt', 'w', encoding='utf-8') as file:
            first = True
            for code_point in unicode_scalar_values(): # encode every usv

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