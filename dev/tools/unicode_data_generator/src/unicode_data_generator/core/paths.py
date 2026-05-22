from pathlib import Path


def project_root() -> Path:
    return Path(__file__).resolve().parents[3]


def default_cache_dir() -> Path:
    return project_root() / '.cache'

def default_output_dir() -> Path:
    return project_root() / 'output'