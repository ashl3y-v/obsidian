#!/usr/bin/env python
"""
Bootloader Build Tool

This tool is responsible for setting up the firmware location and compiling C source code to generate a bootloader binary file.
"""

import argparse
import os
import pathlib
import shutil

from Crypto.PublicKey import ECC
from Crypto.Random import get_random_bytes
from subprocess import run

ROOT_DIR = pathlib.Path(__file__).parent.parent.absolute()
TOOL_DIR = pathlib.Path(__file__).parent.absolute()
BOOTLOADER_DIR = ROOT_DIR / "bootloader"
FIRMWARE_DIR = ROOT_DIR / "firmware/firmware"

SECRET_ERROR = -1
FIRMWARE_ERROR = -2


def copy_initial_firmware(binary_path):
    # Navigate to our tool directory
    os.chdir(TOOL_DIR)

    # Copy generated firmware to build directory
    try:
        shutil.copy(binary_path, BOOTLOADER_DIR / "src/firmware.bin")
    except Exception as excep:
        print(f"{excep}")
        exit(FIRMWARE_ERROR)


def make_bootloader() -> bool:
    # Navigate to bootloader directory
    os.chdir(BOOTLOADER_DIR)

    # Delete any previous executables and compile
    run("make clean", shell=True)

    # Error checking
    status = run("make").returncode
    if status == 0:
        print(f"Compiled binary located at {BOOTLOADER_DIR}")
    else:
        print("Failed to compile, check output for errors.")

    # Return True if make returned 0, otherwise return False.
    return status == 0


def generate_secrets():
    try:
        # Get the directory to store generated files
        crypto = BOOTLOADER_DIR / "crypto"

        # Generate our random symmetric AES key & initalization vector
        aes = get_random_bytes(32)
        iv = get_random_bytes(16)

        # ECC key pair generation
        ecc_private = ECC.generate(curve="secp256r1")
        ecc_public = ecc_private.public_key()

        # Write our AES and ECC private key and close for safety
        with open(crypto / "secret_build_output.txt", mode="wb") as file:
            file.write(aes + b"\n")
            file.write(ecc_private.export_key(format="DER"))

        # Create a .RAW file to store our RAW public key
        with open(crypto / "ecc_public.raw", mode="wb") as file:
            file.write(ecc_public.export_key(format="raw") + b"\n")

        # Lastly, store our IV
        with open(crypto / "iv.txt", mode="wb") as file:
            file.write(iv + b"\n")

    # No point of trying to compile if we don't have any secrets
    except Exception as excep:
        print(f"ERROR: Error while attempting to generate build secrets.\n{excep}")
        exit(SECRET_ERROR)
    else:
        print("All build secrets were generated succesfully.")


def main(args):
    # Build and use default firmware if none is provided,
    # otherwise look for the binary at the path specified
    if args.initial_firmware is None:
        binary_path = FIRMWARE_DIR / "gcc/main.bin"
        os.chdir(FIRMWARE_DIR)

        run("make clean", shell=True)
        run("make")
    else:
        binary_path = os.path.abspath(pathlib.Path(args.initial_firmware))

    # File doesn't exist/cannot be found
    if not os.path.isfile(binary_path):
        raise FileNotFoundError(
            'ERROR: {} does not exist or is not a file. You may have to call "make" in the firmware directory.'.format(
                binary_path
            )
        )

    generate_secrets()
    copy_initial_firmware(binary_path)
    make_bootloader()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Bootloader Build Tool")
    parser.add_argument(
        "--initial-firmware",
        help="Path to the the firmware binary.",
    )
    args = parser.parse_args()

    main(args)
