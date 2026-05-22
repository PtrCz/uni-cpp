from ..add_unicode_version_argument import add_unicode_version_argument
from ...core.datasets import datasets

def register(sub):
    parser = sub.add_parser(
        'analyze',
        help='Analyze dataset structure and statistics'
    )

    add_unicode_version_argument(parser)

    parser.add_argument(
        'dataset',
        choices=['all', *[d.name for d in datasets()]],
        help='Choose which dataset to analyze',
    )

    parser.add_argument(
        '--no-cache',
        action='store_true',
        help='Ignore cache and regenerate everything'
    )