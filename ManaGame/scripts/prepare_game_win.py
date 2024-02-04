# Copies various files needed by ManaGame, to the game folder.
# Run help to see usage:
#   `python prepare_game_win.py -h`

import argparse
import glob
import os
from pathlib import Path
import shutil
import sys

arch = 'x64'        # x64
config = 'debug'    # debug, profile, release

configs = {
    'x64_debug__game_config':       'x64Debug',
    'x64_debug__ogg_config':        'Debug',
    'x64_debug__ogg_arch':          'x64',

    'x64_profile__game_config':     'x64Profile',
    'x64_profile__ogg_config':      'Debug',
    'x64_profile__ogg_arch':        'x64',

    'x64_release__game_config':     'x64Release',
    'x64_release__ogg_config':      'Release',
    'x64_release__ogg_arch':        'x64',
}

def get_config(key: str) -> str:
    return configs[f'{arch}_{config}__{key}']

# Get path containing this script,
# regardless of current working directory
def get_script_path() -> str:
    return os.path.dirname(os.path.realpath(__file__))

def delete_files_and_folders_under_path(path: str, dry_run: bool):
    with os.scandir(path) as entries:
        for entry in entries:
            if dry_run:
                print(f'would delete: {entry.path}')
            else:
                if entry.is_dir() and not entry.is_symlink():
                    shutil.rmtree(entry.path)
                else:
                    print(f'deleting {entry.path}')
                    os.remove(entry.path)

def copy_files_glob(src_glob: str, dest: str):
    for file in glob.glob(src_glob):
        # copy2 attempts to preserve metadata
        shutil.copy2(file, dest)

def main() -> int:
    parser = argparse.ArgumentParser(description='Copies various files needed by ManaGame, to the game folder.')
    parser.add_argument('-c', '--config',
                        help='build configuration',
                        choices=['debug', 'profile', 'release'],
                        default='debug')
    args = parser.parse_args()
    config = args.config

    print(f'Preparing ManaGame for {arch}_{config}...')

    script_path = get_script_path()
    third_party_path = f'{script_path}/../../ManaEngine/third-party/'

    game_path = f'{script_path}/../game/'
    assets_path = f'{script_path}/../assets/'
    game_bin_path = f'{script_path}/../bin/{get_config("game_config")}/'
    ogg_path = f'{third_party_path}/liboggvorbis/Dynamic/bin/{get_config("ogg_arch")}/{get_config("ogg_config")}/'

    # create game folder if it doesn't exist
    Path(game_path).mkdir(exist_ok=True)

    # clean the game folder
    print('Cleaning the game folder...')
    delete_files_and_folders_under_path(game_path, False)

    # copy files to game folder
    print('Copying files to game folder...')
    should_copy_pdb_files = (config == 'debug' or config == 'profile')

    print('  assets/final')
    shutil.copytree(f'{assets_path}final/', game_path, dirs_exist_ok=True)

    print('  ogg-vorbis dlls')
    copy_files_glob(f'{ogg_path}*.dll', game_path)

    print('  game_bin/*.exe')
    copy_files_glob(f'{game_bin_path}*.exe', game_path)

    # xaudio dll is already in the game_bin_path, because it's nuget
    # package puts it there for us during the build.
    print('  game_bin/*.dll')
    copy_files_glob(f'{game_bin_path}*.dll', game_path)

    if should_copy_pdb_files:
        print('  game_bin/*.pdb')
        copy_files_glob(f'{game_bin_path}*.pdb', game_path)

    # TODO: For testing only. Remove before final build.
    #print('  non-versioned assets - REMOVE BEFORE FINAL BUILD')
    #shutil.copytree(f'{assets_path}non-versioned/', game_path, dirs_exist_ok=True)

    return 0

if __name__ == '__main__':
    sys.exit(main())
