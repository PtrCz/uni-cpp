# Parses data from the Unicode Character Database files

from load_ucd_file import load_ucd_file

def is_empty_line(line: str) -> bool:
    return line.strip() == ''

def strip_comments(lines: list[str]) -> list[str]:
    result = []

    for line in lines:
        if '#' not in line:
            result.append(line)
        else:
            idx = line.index('#')
            result.append(line[:idx]) # strip comment
    
    return result

def init_data() -> dict:
    print('[*] Initializing data')

    data = dict()

    for code_point in range(0, 0x110000):
        data[code_point] = { # Initialize with default values
            'uppercase_mapping': [code_point],
            'lowercase_mapping': [code_point],
            'titlecase_mapping': [code_point],
            'properties': set()
        }

    return data

def update_data_with_unicode_data_file(unicode_version: str, code_point_data: dict) -> dict:
    print('[*] Parsing \'UnicodeData.txt\' file')

    lines = load_ucd_file(unicode_version, 'UnicodeData.txt').splitlines()

    for i, line in enumerate(lines):
        if is_empty_line(line): # skip empty lines
            continue
        
        data_fields = line.split(';')
        listed_code_point = int(data_fields[0], 16)

        code_points = [listed_code_point] # One line can store data for a range of code points (see https://www.unicode.org/reports/tr44/#Code_Point_Ranges)

        if data_fields[1].startswith('<') and data_fields[1].endswith(', First>'): # code point starting a range
            next_line = lines[i + 1]
            last_code_point_in_range = int(next_line.split(';')[0], 16)

            code_points.extend(range(listed_code_point + 1, last_code_point_in_range))

        for code_point in code_points:
            simple_uppercase_mapping = int(data_fields[12], 16) if len(data_fields[12]) != 0 else code_point
            simple_lowercase_mapping = int(data_fields[13], 16) if len(data_fields[13]) != 0 else code_point
            simple_titlecase_mapping = int(data_fields[14], 16) if len(data_fields[14]) != 0 else simple_uppercase_mapping

            code_point_data[code_point]['uppercase_mapping'] = [simple_uppercase_mapping]
            code_point_data[code_point]['lowercase_mapping'] = [simple_lowercase_mapping]
            code_point_data[code_point]['titlecase_mapping'] = [simple_titlecase_mapping]
    
    return code_point_data

def update_data_with_special_casing(unicode_version: str, code_point_data: dict) -> dict:
    print('[*] Parsing \'SpecialCasing.txt\' file')

    lines = load_ucd_file(unicode_version, 'SpecialCasing.txt').splitlines()
    lines = strip_comments(lines)

    for line in lines:
        if is_empty_line(line):
            continue

        data_fields = line.split(';')
        data_fields = [field.strip(' ') for field in data_fields] # strip spaces from the fields

        if data_fields[4] != '': # skip context and language dependent mappings
            continue

        code_point = int(data_fields[0], 16)

        get_code_points = lambda field: [int(value, 16) for value in field.split(' ')] # multiple code points are separated by spaces

        code_point_data[code_point]['lowercase_mapping'] = get_code_points(data_fields[1])
        code_point_data[code_point]['titlecase_mapping'] = get_code_points(data_fields[2])
        code_point_data[code_point]['uppercase_mapping'] = get_code_points(data_fields[3])

    return code_point_data

def update_data_with_derived_core_properties(unicode_version: str, code_point_data: dict) -> dict:
    print('[*] Parsing \'DerivedCoreProperties.txt\' file')

    lines = load_ucd_file(unicode_version, 'DerivedCoreProperties.txt').splitlines()
    lines = strip_comments(lines)

    for line in lines:
        if is_empty_line(line):
            continue

        data_fields = line.split(';')
        data_fields = [field.strip(' ') for field in data_fields] # strip spaces from the fields

        code_points = []

        if data_fields[0].find('..') != -1: # range of code points (see https://www.unicode.org/reports/tr44/#Code_Point_Ranges)
            index = data_fields[0].find('..')

            range_start = int(data_fields[0][:index], 16)
            range_end = int(data_fields[0][index + 2:], 16)

            code_points.extend(range(range_start, range_end + 1))
        else: # a single code point
            code_points = [int(data_fields[0], 16)]

        for code_point in code_points:
            code_point_data[code_point]['properties'].add(data_fields[1])

    return code_point_data

def load_unicode_data(unicode_version: str) -> dict:
    """
    Loads the Unicode data from the Unicode Character Database for a given Unicode version.

    The data is returned as a dictionary where each key corresponds to a code point in the range 0x0000 to 0x10FFFF,
    and the value is another dictionary containing information about that code point.

    Args:
        unicode_version (str): The version of Unicode to load data for (e.g., '16.0.0').

    Returns:
        dict: A dictionary representing the loaded Unicode data.
            - The keys in this dictionary are integers in the range 0x0000 to 0x10FFFF, inclusive,
              and correspond to a Unicode code point.
            - Each value is another dictionary containing information about that code point.
              This inner dictionary has the following key-value pairs:
                - 'lowercase_mapping': list of integers representing the lowercase mapping code points (len: [1, 3])
                - 'uppercase_mapping': list of integers representing the uppercase mapping code points (len: [1, 3])
                - 'titlecase_mapping': list of integers representing the titlecase mapping code points (len: [1, 3])
                - 'properties': set of strings representing the properties of this code point. See https://www.unicode.org/reports/tr44/#DerivedCoreProperties.txt for possible values.
    """

    data = init_data()

    for update_func in (
        update_data_with_unicode_data_file,       # UnicodeData.txt
        update_data_with_special_casing,          # SpecialCasing.txt
        update_data_with_derived_core_properties, # DerivedCoreProperties.txt
    ):
        data = update_func(unicode_version, data)

    return data