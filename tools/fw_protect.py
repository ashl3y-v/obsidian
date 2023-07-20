"""
Firmware Bundle-and-Protect Tool

"""
import argparse
import struct

from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from Crypto.Signature import eddsa
from Crypto.PublicKey import ECC

TOOL_DIRECTORY = pathlib.Path(__file__).parent.absolute()

# sender
# data = b"sussy among us"
#
#
# en_key = get_random_bytes(16)
#
# cipher = AES.new(en_key, AES.MODE_GCM)
# nonce = cipher.nonce
#
# ciphertext, tag = cipher.encrypt_and_digest(data + signature)

# reader

# verifier = eddsa.new(eddsa.import_public_key(pub_key.export_key(format="raw")), 'rfc8032')
#
# verifier.verify(data, signature)
#
# cipher = AES.new(en_key, AES.MODE_GCM, nonce=nonce)
#
# plaintext = cipher.decrypt_and_verify(ciphertext, tag)


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
    
    # Determine the directory where cryptographic keys are stored
    cryptography = TOOL_DIRECTORY / '..' / 'bootloader' / 'crypto'

    # Extract keys from build output (AES then ECC)
    sig_key, pub_key, en_key = None
    with open(cryptography / 'secret_build_output.txt', mode='rb') as fp:
        en_key = fp.readline()
        sig_key = ECC.import_key(fp.readline())
        pub_key = sig_key.public_key()

    # Extract automatically generated initalization vector
    iv = None
    with open(cryptography / 'iv.txt', mode='rb') as fp:
        iv = fp.readline()

    signer = eddsa.new(sig_key, mode="rfc8032")
    signature = signer.sign(data)

    # check if supports 128 bit nonce, C impl might be cringe
    # nonce = get_random_bytes(12)
    # GCM mode ftw ong
    # padding issues ???? (sus)
    cipher = AES.new(en_key, AES.MODE_GCM, iv=iv)
    nonce = cipher.nonce

    # MAC tag 4 checking integrity
    ciphertext, tag = cipher.encrypt_and_digest(firmware_blob)

    # Write firmware blob to outfile
    # :( bye bye firmware blob nice knowing you
    with open(outfile, "wb+") as outfile:
        outfile.write(firmware_blob)

    print(sig_key.export_key(mode="raw"))
    with open("secret_build_output.txt", "wb") as secfile:
        # do we need to save private key?
        secfile.write(en_key + b"\n" + sig_key.export_key(mode="raw") + b"\n")

    with open("public.txt", "wb") as pubfile:
        pubfile.write(pub_key.export_key(mode="raw") + b"\n")

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
