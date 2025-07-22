import pathlib

def analyse_case_data(unicode_version: str, code_point_data: dict):
    """
    Analyse the provided code point case data and provide a dictionary with the results.

    Saves the analysis to a file and returns it as a dictionary.
    
    The analysis results contain analysis for each case type, including:

    - Count of mappings
    - Greatest code point with a mapping
    - Offset information (unique offsets in simple mappings)
    - Distinct titlecase mappings from uppercase mappings information
    """

    print('[*] Analysing case mapping data')

    cases = ['lowercase', 'uppercase', 'titlecase'] # Note: do not change the order!
    mapping_types = ['mappings', 'simple_mappings', 'special_mappings'] # Note: do not change the order!

    # Initialize the data

    case_data = dict()
    for case in cases:
        case_data[case] = dict()

        for mapping_type in mapping_types:
            case_data[case][mapping_type] = {
                'count': 0,
                'unique_mappings': [], # a list of all the unique mappings for a given case and mapping type
                'greatest_code_point_with_mapping': 0
            }

        case_data[case]['simple_mappings']['offsets'] = {0}
        
    for mapping_type in mapping_types:
        case_data['titlecase'][mapping_type]['distinct_from_uppercase'] = 0

    # Analyse the data

    for code_point in code_point_data:
        for case in cases:
            mapping = code_point_data[code_point][case + '_mapping']

            if len(mapping) != 1 or mapping[0] != code_point: # the code point doesn't map to itself
                case_data[case]['mappings']['count'] += 1

                if mapping not in case_data[case]['mappings']['unique_mappings']:
                    case_data[case]['mappings']['unique_mappings'].append(mapping)

                case_data[case]['mappings']['greatest_code_point_with_mapping'] = max(code_point, case_data[case]['mappings']['greatest_code_point_with_mapping'])

                if len(mapping) != 1: # special mapping
                    case_data[case]['special_mappings']['count'] += 1

                    if mapping not in case_data[case]['special_mappings']['unique_mappings']:
                        case_data[case]['special_mappings']['unique_mappings'].append(mapping)
                        
                    case_data[case]['special_mappings']['greatest_code_point_with_mapping'] = max(code_point, case_data[case]['special_mappings']['greatest_code_point_with_mapping'])

                else: # simple mapping
                    case_data[case]['simple_mappings']['count'] += 1

                    if mapping not in case_data[case]['simple_mappings']['unique_mappings']:
                        case_data[case]['simple_mappings']['unique_mappings'].append(mapping)

                    case_data[case]['simple_mappings']['greatest_code_point_with_mapping'] = max(code_point, case_data[case]['simple_mappings']['greatest_code_point_with_mapping'])
                    case_data[case]['simple_mappings']['offsets'].add(mapping[0] - code_point)

        uppercase_mapping = code_point_data[code_point]['uppercase_mapping']
        titlecase_mapping = code_point_data[code_point]['titlecase_mapping']

        if titlecase_mapping != uppercase_mapping:
            case_data['titlecase']['mappings']['distinct_from_uppercase'] += 1

            if len(mapping) != 1: # special mapping
                case_data['titlecase']['special_mappings']['distinct_from_uppercase'] += 1
            else: # simple mapping
                case_data['titlecase']['simple_mappings']['distinct_from_uppercase'] += 1

    # Save the data to a file

    out_filepath = f'output/{unicode_version}/analysis'

    pathlib.Path(out_filepath).mkdir(parents=True, exist_ok = True)
    
    with open(out_filepath + '/case_mappings.txt', 'w', encoding='utf-8') as file:
        for mapping_type_index, mapping_type_prefix in enumerate(['', 'simple ', 'special ']):
            mapping_type = mapping_types[mapping_type_index]

            for case in cases:
                file.write(f'{mapping_type_prefix}{case} mappings: {case_data[case][mapping_type]['count']} ({len(case_data[case][mapping_type]['unique_mappings'])} unique)')
                if (case == 'titlecase'):
                    file.write(f' ({case_data[case][mapping_type]['distinct_from_uppercase']} mappings distinct from the uppercase mappings)')
                
                file.write('\n')

            file.write('\n')

        for case in cases:
            greatest_code_point_with_mapping = case_data[case]['mappings']['greatest_code_point_with_mapping']
            a = 'an' if case == 'uppercase' else 'a'
            
            file.write(f'greatest code point with {a} {case} mapping: U+{greatest_code_point_with_mapping:06X}\n')

        file.write('\n')

        for case in cases:
            offsets = case_data[case]['simple_mappings']['offsets']
            file.write(f'unique offsets in simple {case} mappings: (COUNT = {len(offsets)}) {str(sorted(offsets))}\n')

    return case_data