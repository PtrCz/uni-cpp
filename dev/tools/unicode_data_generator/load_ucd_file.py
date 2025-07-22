import urllib.request
import os
import pathlib

def download_ucd_file(unicode_version: str, filename: str, out_filepath: str):
    file_url = 'https://www.unicode.org/Public/' + unicode_version + '/ucd/' + filename

    print('[*] Downloading file from \'' + file_url + '\' to \'' + out_filepath + '\'')

    contents = urllib.request.urlopen(file_url).read().decode('utf-8')

    with open(out_filepath, 'w', encoding='utf-8') as out_file:
        out_file.write(contents)


def load_ucd_file(unicode_version: str, filename: str) -> str:
    """
    Loads a Unicode Character Database (UCD) file as plain text.

    Downloads the file from the Unicode website for the specified version,
    or uses a cached version if available.

    Args:
        unicode_version (str): The Unicode version to load (e.g., '16.0.0').
        filename (str): The name of the UCD file to load (e.g., 'UnicodeData.txt').
    """

    ucd_file_dir = 'temp/UCD/' + unicode_version

    filepath = ucd_file_dir + '/' + filename

    if os.path.exists(filepath): # The file has been downloaded already
        return pathlib.Path(filepath).read_text(encoding='utf-8')

    pathlib.Path(ucd_file_dir).mkdir(parents = True, exist_ok = True) # Make sure the directories exist

    download_ucd_file(unicode_version, filename, filepath)

    return pathlib.Path(filepath).read_text(encoding='utf-8')