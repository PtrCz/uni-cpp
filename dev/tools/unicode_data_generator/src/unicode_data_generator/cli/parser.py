import argparse

from ..core.paths import default_cache_dir, default_output_dir

from .commands import (
    generate,
    analyze,
    clean,
    list,
)

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog='unicode_data_generator',
        description=f'Unicode data generator for uni-cpp',
    )

    output = parser.add_argument_group('output')

    output.add_argument(
        '--cache-dir',
        default=default_cache_dir(),
        help='Override cache directory',
    )

    output.add_argument(
        '--output-dir',
        default=default_output_dir(),
        help='Override output directory',
    )

    sub = parser.add_subparsers(
        dest='command',
        required=True,
    )

    generate.register(sub)
    analyze.register(sub)
    clean.register(sub)
    list.register(sub)

    return parser