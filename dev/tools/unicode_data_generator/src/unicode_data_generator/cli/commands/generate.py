from ..add_unicode_version_argument import add_unicode_version_argument
from ...core.datasets import datasets_and_all, test_datasets_and_all

def register(p):
    parser = p.add_parser(
        'generate',
        help='Generate Unicode tables & test data'
    )

    sub = parser.add_subparsers(
        dest='target',
        required=True,
    )

    register_all_subparser(sub)
    register_tables_subparser(sub)
    register_tests_subparser(sub)


def register_all_subparser(sub):
    parser = sub.add_parser(
        'all',
        help='Generate all C++ tables & test data',
    )

    add_unicode_version_argument(parser)
    add_precomputed_tuning_argument(parser)


def register_tables_subparser(sub):
    parser = sub.add_parser(
        'tables',
        help='Generate C++ tables',
    )

    parser.add_argument(
        'dataset',
        choices=datasets_and_all(),
        help='Choose which data tables to generate',
    )

    add_unicode_version_argument(parser)
    add_precomputed_tuning_argument(parser)


def register_tests_subparser(sub):
    parser = sub.add_parser(
        'tests',
        help='Generate tests',
    )

    parser.add_argument(
        'dataset',
        choices=test_datasets_and_all(),
        help='Choose which tests to generate',
    )

    add_unicode_version_argument(parser)


def add_precomputed_tuning_argument(parser):
    parser.add_argument(
        '--use-precomputed-tuning',
        action='store_true',
        help='Use precomputed (hardcoded) optimal block sizes instead of running block-size fine-tuning',
    )