import os
import sys
import glob
import argparse
import subprocess


TEMP_FORMAT_TARGETS_FILE: str = 'format-targets.txt'


def get_clang_format_executable():
    if sys.platform == 'win32':
        return 'clang-format.exe'
    return 'clang-format'


def scan_directory(path: str, extension: str = '*pp') -> list[str]:
    return glob.glob(f'**/*.{extension}', root_dir=path, recursive=True)


def make_command(argv: argparse.Namespace) -> list[str]:
    return [
        get_clang_format_executable(),
        '--files', TEMP_FORMAT_TARGETS_FILE,
        '--dry-run' if argv.dry else '-i'
    ]


def main(argv: argparse.Namespace) -> int:
    with open(TEMP_FORMAT_TARGETS_FILE, 'w+') as file:
        for target in scan_directory(argv.directory):
            file.write(os.path.join(argv.directory, target))
            file.write('\n')

    exit_code: int = 0
    try:
        cmd: list[str] = make_command(argv)
        subprocess.run(cmd)
    except Exception as e:
        print(f'Fatal error: {str(e)}')
        exit_code = -1

    os.remove(TEMP_FORMAT_TARGETS_FILE)
    return exit_code


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='run-clang-format',
        description='Run clang-format for the codebase'
    )
    parser.add_argument('directory',
                        help='The root directory of formatting code');

    parser.add_argument('--dry',
                        action='store_true',
                        help='Do not apply formatting to source files');
    exit(main(parser.parse_args()))
