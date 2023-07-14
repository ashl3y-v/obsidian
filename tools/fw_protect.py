"""
Firmware Bundle-and-Protect Tool

"""
import argparse
import struct

from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes


def protect_firmware(infile, outfile, version, message):
    # Load firmware binary from infile
    with open(infile, "rb") as fp:
        firmware = fp.read()

    # Append null-terminated message to end of firmware
    firmware_and_message = firmware + message.encode() + b"\00"

    # Pack version and size into two little-endian shorts
    metadata = struct.pack("<HH", version, len(firmware))

    # Append firmware and message to metadata
    firmware_blob = metadata + firmware_and_message

    # random key, duh
    key = get_random_bytes(256)

    # check if supports 128 bit nonce, C impl might be cringe
    # nonce = get_random_bytes(12)
    # GCM mode ftw ong
    # padding issues ???? (sus)
    cipher = AES.new(key, AES.MODE_GCM)
    nonce = cipher.nonce

    # MAC tag 4 checking integrity
    ciphertext, tag = cipher.encrypt_and_digest(firmware_blob)

    # Write firmware blob to outfile
    # :( bye bye firmware blob nice knowing you
    with open(outfile, "wb+") as outfile:
        outfile.write(firmware_blob)

    # now I'm all alone, my scope is at an end


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
