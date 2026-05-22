def datasets() -> list[str]:
    return [
        'case_mapping',
    ]

def datasets_and_all() -> list[str]:
    ds = datasets()
    ds.insert(0, 'all')

    return ds

def test_datasets() -> list[str]:
    return [
        'utf_encoding',
        'case_mapping'
    ]

def test_datasets_and_all() -> list[str]:
    ds = test_datasets()
    ds.insert(0, 'all')

    return ds