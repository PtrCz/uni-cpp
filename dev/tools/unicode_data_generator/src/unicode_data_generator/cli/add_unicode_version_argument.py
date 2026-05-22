import argparse
import re

MAJOR_MINOR = re.compile(r'^\d+\.\d+$')
FULL_SEMVER = re.compile(r'^\d+\.\d+\.\d+$')

def parse_unicode_version(value: str) -> str:
    if value == 'latest':
        return value

    if FULL_SEMVER.match(value):
        return value

    if MAJOR_MINOR.match(value):
        major, minor = value.split('.')
        return f'{major}.{minor}.0'

    raise argparse.ArgumentTypeError(
        'Invalid --unicode-version. Expected \'latest\' or a semantic version (like 17.0.0)'
    )

def add_unicode_version_argument(parser, *, required=True, help='Unicode version to use (e.g. 17.0.0 or latest)'):
    parser.add_argument(
        '-u', '--unicode-version',
        type=parse_unicode_version,
        required=required,
        help=help,
    )