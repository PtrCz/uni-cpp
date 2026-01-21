from generate_tables import generate_tables

def generate_fine_tuned_tables(prop_value_list: list, prop_value_type_size: int) -> dict:
    """
    This function generates fine-tuned multistage lookup tables for a given property value list and type size.
    This function can be quite slow, as it iteratively checks different block sizes to find the most optimal one that results in the smallest total size.

    See `generate_tables` function from `generate_tables.py` and it's documentation for more.
    """

    print('[*] Fine-tuning the block size of the multistage lookup tables.')

    calc_total_size_for_given_block_size = lambda block_size: generate_tables(block_size, prop_value_list, prop_value_type_size)['total_size']

    step = 64
    greatest_block_size_initially_checked = 1024

    # Length of the progress bar in the console
    bar_len = 60

    # The total number of calls made to `generate_tables` function while fine-tuning.
    # The `greatest_block_size_initially_checked // step` is from the block_sizes definition below and the `15 * 2` is from 2 loop iterations below. 
    total_check_count = 15 * 2 + greatest_block_size_initially_checked // step
    current_check_count = 0

    block_sizes = [n * step for n in range(1, greatest_block_size_initially_checked // step + 1)]

    # Forwards the call to `calc_total_size_for_given_block_size` and tracks the progress.
    def counting_key(block_size):
        nonlocal current_check_count
        current_check_count += 1

        ratio = current_check_count / total_check_count

        done = int(bar_len * ratio)
        bar  = '#' * done + '-' * (bar_len - done)

        print(f'[@] [{bar}] {ratio * 100:6.2f}%', end='\r', flush=True)
        
        return calc_total_size_for_given_block_size(block_size)

    # Calculates the total size of tables for each block size in `block_sizes` and returns the `block_size` resulting in the smallest total size
    best_block_size = min(block_sizes, key=counting_key)

    # Increase the precision with each iteration
    while step >= 8:
        prev_step = step
        step //= 8

        block_sizes = [n * step + best_block_size - prev_step for n in range(1, 16)]

        best_block_size = min(block_sizes, key=counting_key)

    print(' ' * 150, end='\r', flush=True) # clear the console line from the progress bar

    print(f'[+] Most optimal block size found: {best_block_size}')
    return generate_tables(best_block_size, prop_value_list, prop_value_type_size)