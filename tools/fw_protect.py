"""
Firmware Bundle-and-Protect Tool

"""
import argparse
import struct
import pathlib

from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from Crypto.Signature import eddsa
from Crypto.PublicKey import ECC

TOOL_DIRECTORY = pathlib.Path(__file__).parent.absolute()


def protect_firmware(infile, outfile, version, message):
    # Load firmware binary from infile
    with open(infile, "rb") as infile:
        firmware = infile.read()

    # Determine the directory where cryptographic keys are stored
    key_dir = TOOL_DIRECTORY / ".." / "bootloader" / "crypto"

    # Extract keys from build output (AES then ECC)
    with open(key_dir / "secret_build_output.txt", mode="rb") as privfile:
        aes_key = privfile.readline().strip()
        priv_key = ECC.import_key(privfile.readline().strip())

    # Extract automatically generated initalization vector
    with open(key_dir / "iv.txt", mode="rb") as ivfile:
        nonce = ivfile.readline().strip()

    message = message.encode()

    # Pack version and size into two little-endian shorts
    metadata = (
        struct.pack("<HH", version, len(message))
        + message
        + struct.pack("<H", len(firmware))
    )

    aes = AES.new(aes_key, AES.MODE_GCM, nonce=nonce)
    signer = eddsa.new(priv_key, mode="rfc8032")

    blob = metadata + aes.encrypt(firmware)
    blob = signer.sign(blob) + blob

    with open(outfile, "wb+") as outfile:
        outfile.write(blob)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Firmware Update Tool")
    parser.add_argument(
        "--infile", help="Path to the firmware image to protect.", required=True
    )
    parser.add_argument(
        "--outfile", help="Filename for the output firmware.", required=True
    )
    parser.add_argument(
        "--version", help="Version number of this firmware.", required=True
    )
    parser.add_argument(
        "--message", help="Release message for this firmware.", required=True
    )
    args = parser.parse_args()

    protect_firmware(
        infile=args.infile,
        outfile=args.outfile,
        version=int(args.version),
        message=args.message,
    )
