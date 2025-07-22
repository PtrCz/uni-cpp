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

