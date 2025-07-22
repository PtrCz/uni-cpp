# from: https://stackoverflow.com/a/17870684
def find_sublist(sublist, list):
    sublist_length = len(sublist)

    for ind in (i for i,e in enumerate(list) if e==sublist[0]):
        if list[ind:ind + sublist_length]==sublist:
            return ind

def overlap(a, b):
    max_overlap = 0
    for i in range(1, min(len(a), len(b)) + 1):
        if a[-i:] == b[:i]:
            max_overlap = i
    return max_overlap

def merge_arrays(arr1, arr2):
    overlap_ab = overlap(arr1, arr2)
    overlap_ba = overlap(arr2, arr1)
    if overlap_ab >= overlap_ba:
        return arr1 + arr2[overlap_ab:]
    else:
        return arr2 + arr1[overlap_ba:]

def shortest_superarray(arrays):
    while len(arrays) > 1:
        max_olap = -1
        best_pair = (0, 1)
        merged = []
        for i in range(len(arrays)):
            for j in range(len(arrays)):
                if i != j:
                    merged_candidate = merge_arrays(arrays[i], arrays[j])
                    olap = len(arrays[i]) + len(arrays[j]) - len(merged_candidate)
                    if olap > max_olap:
                        max_olap = olap
                        best_pair = (i, j)
                        merged = merged_candidate
        i, j = best_pair
        new_arrays = [arrays[k] for k in range(len(arrays)) if k != i and k != j]
        new_arrays.append(merged)
        arrays = new_arrays
    return arrays[0]

def min_byte_count_needed_to_store_value(value: int) -> int:
    if value > 0xFFFFFFFF:
        return 8
    elif value > 0xFFFF:
        return 4
    elif value > 0xFF:
        return 2
    else:
        return 1

def generate_tables(block_size: int, prop_value_list: list, prop_value_type_size: int) -> dict:
    """
    Generate the necessary lookup tables for efficient storage and retrieval of Unicode properties.
    To understand the basic concept of how this works read: https://here-be-braces.com/fast-lookup-of-unicode-properties/.

    Here we use a few extra optimizations where possible:
        - Overlap the blocks as much as possible to save space in stage2. This introduces a lot of complexity to the generation algorithm,
        but it's worth the saved space. This however disables another optimization: if we didn't overlap the blocks, then instead of
        storing indexes into stage2 directly, we could have stored indexes into blocks and then multiply the value from stage1 by block_size.
        Why would this be better? That's because block indexes have smaller values and often could result in making the stage1 integer value type 8-bit
        instead of 16-bit, which would divide the size of stage1 table by 2. Since this optimization isn't possible with block overlap, we have
        another way to shrink the stage1 integer value type and this brings us to the next optimization.
        - Storing unique offsets to stage2 in an extra table. The offsets to stage2 stored in stage1 often don't fit into 8 bits
        and use 16 or 32 bits. Often enough the offsets are greater than 0xFF, but there is less than 0xFF unique offsets. In such cases
        it is better to use an extra table storing the unique offsets to stage2 and have stage1 store indexes into that extra table. This is more efficient,
        because it allows stage1 table to use a smaller value type, while the extra table using the larger value type has way less values.
        - Storing the properties in-place, in stage2 table, instead of having a stage3 table. Suppose the property type size is 1 byte and there
        are 256 unique property values: in this case the stage2 table would store indexes into stage3 table using 1 byte integer type.
        Since the property values are 1 byte and the stage2 value type is 1 byte the stage3 is redundant and it is better to store the property values
        in the stage2 directly instead of storing an index into an extra redundant stage3 table. 

    These optimizations are only used in cases where they actually save space. In some cases, these either don't save space or actually waste space.
    In such cases they are not used. For this reason, different inputs will result in different optimization mixes and the lookup algorithm will differ.

    To understand how to correctly lookup the property values in the generated tables for every optimization and block_size scenario see:
        - `dev/docs/multistage-lookup-tables.md` documentation file.
        - `test_lookup_tables.py` python file.

    Args:
        block_size (int): The size of each block in the table.
        prop_value_list (list): A list of property values for each code point. That is, prop_value_list[0x10] stores the properties for U+0010.
                                This list doesn't have to store properties for the full code point range. In case it doesn't, the lookup
                                will only be valid/safe if it is performed on a code point which properties were stored (i.e. for a list
                                of properties covering the range [U+0000; U+0100], the lookup is only valid on code points in that range). 
        prop_value_type_size (int): The amount of bytes required to store each property value in the table.
                                    For exmaple, if the properties are stored using `uint16_t`s, then the value would be 2.
                                    This value is used to choose the most optimal optimizations.

    Returns:
        dict: A dictionary containing the generated tables and their properties.
            - 'total_size': an integer representing the total size of all generated tables, in bytes.
            - 'block_size' (int): an integer representing the block size used to generate the tables. Equal to the `block_size` parameter.
            - 'stage1': a list of stage 1 table entries.
            - 'stage2_offsets' (optional): a list of stage 2 offsets. Not always used (see 'format').
            - 'stage2': a list of stage 2 table entries (overlapped blocks).
            - 'stage3' (optional): a list of stage 3 table entries. Not always used (see 'format').
            - 'format': a dictionary containing the format of the generated tables and the used optimizations:
                - 'stage1_needs_extra_lookup': bool indicating whether stage 1 requires an extra lookup in an extra lookup table.
                - 'stage2_holds_prop_values_inplace': bool indicating whether stage 2 holds property values in-place.
                - 'stage1_value_type_size', 'stage2_offsets_value_type_size', 'stage2_value_type_size' and 'stage3_value_type_size' are
                    integers representing the size of the integer type needed to store the values of each table, in bytes.
    """

    tables = {
        'stage1': [],
        'stage2_offsets': [], # optional; Not always used.
        'stage2': [],
        'stage3': [], # optional; Not always used.
        'format': {
            'stage1_needs_extra_lookup': False, # if True, stage1 holds indexes into an extra stage2_offsets lookup table, instead of offsets into stage2 directly
            'stage2_holds_prop_values_inplace': False, # stage 2 holds property values instead of indexes into stage3, because it's more efficient
            'stage1_value_type_size': 1,
            'stage2_offsets_value_type_size': 2,
            'stage2_value_type_size': 1,
            'stage3_value_type_size': prop_value_type_size,
        },
        'block_size': block_size,
        'total_size': 0,
    }

    current_block = []
    blocks = []

    stage1_block_indexes = []

    for prop_value in prop_value_list:
        
        if prop_value in tables['stage3']:
            stage3_index = tables['stage3'].index(prop_value)
        else:
            stage3_index = len(tables['stage3'])
            tables['stage3'].append(prop_value)

        current_block.append(stage3_index)

        if len(current_block) == block_size:

            if current_block in blocks:
                block_index = blocks.index(current_block)
            else:
                block_index = len(blocks)
                blocks.append(current_block)

            stage1_block_indexes.append(block_index)

            current_block = []

    # if range_end % block_size != 0, then the last block wasn't properly inserted. Do it here

    if len(current_block) != 0:
        block_index = len(blocks)
        blocks.append(current_block)

        stage1_block_indexes.append(block_index)
    
    # overlap the blocks as much as possible to compress the data

    overlapped = shortest_superarray(blocks)
    tables['stage2'] = overlapped

    for block_index in stage1_block_indexes:
        tables['stage1'].append(find_sublist(blocks[block_index], overlapped))

    # calculate the value type size of tables

    greatest_value = 0
    for value in tables['stage1']:
        greatest_value = max(value, greatest_value)

    tables['format']['stage1_value_type_size'] = min_byte_count_needed_to_store_value(greatest_value)

    greatest_value = 0
    for value in tables['stage2']:
        greatest_value = max(value, greatest_value)

    tables['format']['stage2_value_type_size'] = min_byte_count_needed_to_store_value(greatest_value)

    # check for possible optimizations (see `format` above for the optimizations)

    if tables['format']['stage1_value_type_size'] > 1:
        unique_stage2_offsets = sorted(set(tables['stage1']))

        stage1_current_size = len(tables['stage1']) * tables['format']['stage1_value_type_size']

        stage1_new_size = len(tables['stage1']) * min_byte_count_needed_to_store_value(len(unique_stage2_offsets) - 1)

        stage2_offsets_size = len(unique_stage2_offsets) * tables['format']['stage1_value_type_size']

        saved_bytes = stage1_current_size - stage1_new_size - stage2_offsets_size

        if saved_bytes > 0:
            tables['stage2_offsets'] = unique_stage2_offsets
            tables['format']['stage2_offsets_value_type_size'] = tables['format']['stage1_value_type_size']
            tables['format']['stage1_value_type_size'] = min_byte_count_needed_to_store_value(len(unique_stage2_offsets) - 1)

            new_stage1 = []
            for stage2_offset in tables['stage1']:
                new_stage1.append(unique_stage2_offsets.index(stage2_offset))

            tables['stage1'] = new_stage1
            tables['format']['stage1_needs_extra_lookup'] = True

    if tables['format']['stage3_value_type_size'] <= tables['format']['stage2_value_type_size']:
        # If the property value type size is smaller or equal to the stage2 index type size, then its best to place the values inplace (in stage2).

        new_stage2 = []
        for stage3_index in tables['stage2']:
            new_stage2.append(tables['stage3'][stage3_index])

        tables['stage2'] = new_stage2

        tables['format']['stage2_value_type_size'] = tables['format']['stage3_value_type_size']
        tables['stage3'] = []
        tables['format']['stage2_holds_prop_values_inplace'] = True

    # calculate the total size

    total_size = 0
    total_size += len(tables['stage1']) * tables['format']['stage1_value_type_size']
    total_size += len(tables['stage2']) * tables['format']['stage2_value_type_size']

    if tables['format']['stage1_needs_extra_lookup']:
        total_size += len(tables['stage2_offsets']) * tables['format']['stage2_offsets_value_type_size']

    if not tables['format']['stage2_holds_prop_values_inplace']:
        total_size += len(tables['stage3']) * tables['format']['stage3_value_type_size']

    tables['total_size'] = total_size
    return tables