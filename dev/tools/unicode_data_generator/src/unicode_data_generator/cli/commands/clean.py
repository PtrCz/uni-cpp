from ..add_unicode_version_argument import add_unicode_version_argument

def register(sub):
    parser = sub.add_parser(
        'clean',
        help='Remove cache and output directories'
    )

    add_unicode_version_argument(
        parser,
        required=False,
        help='Optionally provide a Unicode version for which the files should be cleaned (e.g. 17.0.0 or latest)'
    )

    parser.add_argument(
        'target',
        choices=[
            'all',
            'cache',
            'output',
        ],
        help='Choose which files should be deleted'
    )