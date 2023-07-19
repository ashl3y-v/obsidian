#!/usr/bin/env python

"""
bl_build.py --initial-firmware <binary_path>

This tool is responsible for setting up the firmware location and compiling C source code to generate a bootloader binary file.
"""

import argparse
import os
import pathlib
import shutil
from subprocess import run

TOOL_DIRECTORY = pathlib.Path(__file__).parent.absolute()

def compile_bootloader() -> bool:
    # Navigate to bootloader directory
    bootloader = TOOL_DIRECTORY / '..' / 'bootloader'
    os.chdir(bootloader)

    # Delete any previous executables and compile
    run('make clean', shell=True)

    # Did our compilation succeed
    return run("make") == 0

def copy_firmware(firmware_path: str) -> None:
    # Navigate to our tool directory
    os.chdir(TOOL_DIRECTORY)

    # Copy generated firmware to build directory
    bootloader = TOOL_DIRECTORY / '..' / 'bootloader'
    shutil.copy(firmware_path, bootloader / 'src' / 'firmware.bin')

def generate_secrets():
    # TODO: Generate AES/ECC/RSA keys
    pass

if __name__ == '__main__':

    # Setup arguments for our application
    parser = argparse.ArgumentParser(description='Obsidian Build Tool')
    parser.add_argument("--initial-firmware", help="A path to the compiled firmware binary", default=None)
    args = parser.parse_args()

    # Use the default firmware if none is provided otherwise look for the binary at the path specificed
    if args.initial_firmware is None:
        binary_path = TOOL_DIRECTORY / '..' / 'firmware' / 'firmware' / 'gcc' / 'main.bin'
    else:
        binary_path = os.path.abspath(pathlib.Path(args.initial_firmware))

    # File doesn't exist/cannot be found
    if not os.path.isfile(binary_path):
        raise FileNotFoundError(
            "ERROR: {} does not exist or is not a file. You may have to call \"make\" in the firmware directory."
            .format(binary_path)
        )

    generate_secrets()
    copy_firmware(binary_path)
    compile_bootloader()
    print("Compiled succesfully.")


