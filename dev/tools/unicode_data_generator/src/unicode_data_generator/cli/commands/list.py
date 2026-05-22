def register(sub):
    parser = sub.add_parser(
        'list',
        help='List available options'
    )

    parser.add_argument(
        'target',
        help='Choose what should be listed',
        choices=[
            'datasets',
            'tests',
        ],
    )