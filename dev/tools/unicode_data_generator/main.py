import inspect
import os
import re

from load_unicode_data import load_unicode_data
from analyse_case_data import analyse_case_data
from generate_property_tables import generate_case_conversion_property_tables
from generate_tables import generate_tables
from fine_tune import generate_fine_tuned_tables
from test_lookup_tables import test_lookup_tables
from generate_cpp_file import generate_cpp_file

def is_valid_semver(version: str) -> bool:
    semver_regex = re.compile(
        r'^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$'
    )
    return bool(semver_regex.match(version))


def main():
    os.chdir(os.path.dirname(os.path.abspath(inspect.getsourcefile(lambda:0)))) # Sets the working dir to the directory this source file is in

    while True:
        unicode_version = input('[?] Enter full semantic Unicode version (e.g. 16.0.0): ')

        # validate input
        if is_valid_semver(unicode_version):
            break

    while True:
        fine_tune = input('[?] Fine-tune multistage lookup tables? [Y = \'yes\'; n = \'no, use hardcoded settings tuned for Unicode 16.0.0\'] ')
        fine_tune = fine_tune.strip().lower()

        # validate input
        if fine_tune == '' or fine_tune == 'y' or fine_tune == 'n':
            break

    fine_tune = False if fine_tune == 'n' else True # default to True if input was empty

    code_point_data = load_unicode_data(unicode_version)

    # Generate the analysis files
    case_data = analyse_case_data(unicode_version, code_point_data)

    # Generate the C++ unicode data files
    property_tables = []

    for generate_func in (
        generate_case_conversion_property_tables,
    ):
        prop_tables = generate_func(unicode_version, code_point_data, case_data)
        property_tables.append(prop_tables)

        if fine_tune:
            tables = generate_fine_tuned_tables(prop_tables['prop_value_list'], prop_tables['prop_value_type_size'])
        else:
            tables = generate_tables(prop_tables['hardcoded_optimal_block_size'], prop_tables['prop_value_list'], prop_tables['prop_value_type_size'])


        test_lookup_tables(prop_tables['prop_value_list'], tables)
        
        generate_cpp_file(
            prop_tables['unicode_version'],
            prop_tables['cpp_output_filepath'],
            prop_tables['namespace_name'],
            prop_tables['property_type_name'],
            tables,
            prop_tables['cpp_extra_contents']
            )


if __name__ == '__main__':
    main()