import sys

def test_fail():
    print('[!] Test failed! Generated tables are broken!')
    print('[!] Error occurred! Exiting...')

    sys.exit(1)

def test_lookup_tables(prop_value_list: list, generated_lookup_tables: dict) -> None:
    """
    Validate the accuracy of the generated lookup tables by comparing them against a reference list of properties.

    This method iterates over each property in the provided list and performs a lookup using the generated tables.
    If any discrepancy is found, it displays an error message and terminates the program.
    """

    print('[*] Testing generated lookup tables')

    block_size = generated_lookup_tables['block_size']

    stage1_needs_extra_lookup = generated_lookup_tables['format']['stage1_needs_extra_lookup']

    stage2_holds_prop_values_inplace = generated_lookup_tables['format']['stage2_holds_prop_values_inplace']

    for code_point, property in enumerate(prop_value_list):

        # Lookup the code point property in the generated lookup tables and compare it with the `property` from the list

        try:
            # For the lookup algorithm read `dev/docs/multistage-lookup-tables.md`

            # STAGE 1
            stage1_index = code_point // block_size # `//` performs integer (floor) division
            stage1_value = generated_lookup_tables['stage1'][stage1_index]

            offset = generated_lookup_tables['stage2_offsets'][stage1_value] if stage1_needs_extra_lookup else stage1_value

            # STAGE 2
            stage2_index = offset + code_point % block_size
            stage2_value = generated_lookup_tables['stage2'][stage2_index]

            # STAGE 3
            lookup_result = stage2_value if stage2_holds_prop_values_inplace else generated_lookup_tables['stage3'][stage2_value]
                
        except:
            test_fail()

        # Compare the lookup result with the property from the property list
        if lookup_result != property:
            test_fail()

    print('[+] Test passed')