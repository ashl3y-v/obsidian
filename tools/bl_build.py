#!/usr/bin/env python
"""
Bootloader Build Tool

This tool is responsible for setting up the firmware location and compiling C source code to generate a bootloader binary file.
"""

import argparse
import os
import pathlib
import shutil

from subprocess import run, call
from util import arrayize

from Crypto.PublicKey import ECC
from Crypto.Random import get_random_bytes

# various project directories
ROOT_DIR = pathlib.Path(__file__).parent.parent.absolute()
TOOL_DIR = pathlib.Path(__file__).parent.absolute()
FIRMWARE_DIR = ROOT_DIR / "firmware"
BOOTLOADER_DIR = ROOT_DIR / "bootloader"
CRYPTO_DIR = BOOTLOADER_DIR / "crypto"

# errors which can be thrown
SECRET_ERROR = -1
FIRMWARE_ERROR = -2


def copy_initial_firmware(binary_path):
    # Navigate to our tool directory
    os.chdir(TOOL_DIR)

    # Copy generated firmware to build directory
    try:
        shutil.copy(binary_path, BOOTLOADER_DIR / "src/firmware.bin")
    except Exception:
        exit(FIRMWARE_ERROR)


# compile the bootloader
def make_bootloader(**keys) -> bool:
    # Navigate to bootloader directory
    os.chdir(BOOTLOADER_DIR)

    # Delete any previous executables and compile
    run("make clean", shell=True)

    # Create a make command including all the keys passed
    command = "make "
    variables = [f"{x}='{arrayize(y)}'" for x, y in keys.items()]
    for variable in variables:
        command += variable + " "

    # clean again and make
    call("make clean", shell=True)
    status = call(command, shell=True)

    if status == 0:
        print(f"Compiled binary located at {BOOTLOADER_DIR}")
    else:
        print("Failed to compile, check output for errors.")

    # Return True if make returned 0, otherwise return False.
    return status == 0


def generate_secrets():
    try:
        # Generate our random symmetric AES key & initalization vector
        aes = get_random_bytes(32)
        iv = get_random_bytes(16)

        # ECC key pair generation, curve is NIST P-256
        # export the public key
        ecc_private = ECC.generate(curve="secp256r1")
        ecc_public = ecc_private.public_key()
        exported_public = ecc_public.export_key(format="raw")

        if not os.path.exists(CRYPTO_DIR):
            os.mkdir(CRYPTO_DIR)

        # Write our AES and ECC private key
        with open(CRYPTO_DIR / "secret_build_output.txt", mode="wb") as file:
            file.write(aes)
            file.write(bytes(ecc_private.export_key(format="PEM").encode()))

        # Create a .RAW file to store our RAW public key
        with open(CRYPTO_DIR / "ecc_public.raw", mode="wb") as file:
            file.write(exported_public)

        # Lastly, store our IV
        with open(CRYPTO_DIR / "iv.txt", mode="wb") as file:
            file.write(iv)

        # Write to the secrets.h file
        with open(CRYPTO_DIR / "secrets.h", mode="wb") as file:
            file.write(b"#ifndef SECRETS_H\n")
            file.write(b"#define SECRETS_H\n\n")

            file.write(b"// Needed for ECC Public Key structure\n")
            file.write(b'#include "beaverssl.h"\n\n')

            file.write(b"// Size constants\n")
            file.write(b"#define MAX_VERSION 65535\n")
            file.write(b"#define MAX_MESSAGE_SIZE 1000\n")
            file.write(b"#define MAX_FIRMWARE_SIZE 30000\n")
            file.write(b"#define AES_KEY_LENGTH 32\n")
            file.write(b"#define IV_KEY_LENGTH 16\n")
            file.write(b"#define ECC_KEY_LENGTH 65\n\n")

            file.write(
                f"const uint8_t AES_KEY[AES_KEY_LENGTH] = {arrayize(aes)};\n".encode()
            )
            file.write(f"uint8_t IV_KEY[IV_KEY_LENGTH] = {arrayize(iv)};\n".encode())
            file.write(
                f"const uint8_t ECC_PUBLIC_KEY[ECC_KEY_LENGTH] = {arrayize(exported_public)};\n".encode()
            )
            file.write(b"const br_ec_public_key EC_PUBLIC = {\n")
            file.write(b"\t.curve = BR_EC_secp256r1,\n")
            file.write(b"\t.q = (void*)(ECC_PUBLIC_KEY),\n")
            file.write(b"\t.qlen = sizeof(ECC_PUBLIC_KEY)\n};")
            file.write(b"\n#endif")

        return {"AES_KEY": aes, "IV_KEY": iv, "ECC_PUBLIC_KEY": exported_public}

    # No point of trying to compile if we don't have any secrets
    except Exception as excep:
        print(f"Error while attempting to generate build secrets.\n{excep}")
        exit(SECRET_ERROR)


def main(args):
    # Build and use default firmware if none is provided,
    # otherwise look for the binary at the path specified
    # print("args: ", args.initial_firmware)
    if args.initial_firmware is None:
        binary_path = FIRMWARE_DIR / "gcc" / "main.bin"
        os.chdir(FIRMWARE_DIR)

        run("make clean", shell=True)
        run("make")
    else:
        binary_path = os.path.abspath(pathlib.Path(args.initial_firmware))

    # File doesn't exist/cannot be found
    if not os.path.isfile(binary_path):
        raise FileNotFoundError(
            "ERROR: {} does not exist or is not a file. Make may have failed.".format(
                binary_path
            )
        )

    # Set up secrets
    secrets = generate_secrets()

    # Move the initial firmware
    copy_initial_firmware(binary_path)

    # Run make commands
    make_bootloader(**secrets)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Bootloader Build Tool")
    parser.add_argument("--initial-firmware", help="Path to the the firmware binary.")
    args = parser.parse_args()
    main(args)
