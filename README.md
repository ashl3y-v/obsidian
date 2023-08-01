# OBSIDIAN

# encryption and signing
AES-CBC-256
ECDSA with P-256r

## Setup
Download the code to your machine using:
```$ git clone https://github.com/ash-v3/obsidian.git```

Make sure PyCryptodome is running the latest version as some of the cryptographic functionality used in this repo was added in later versions.

```$ python -m pip install -U pycryptodome```

### Steps
 ``[]`` indicates required arguments, ``<>`` indicates optional arguments.
 - Provide a firmware to be compiled with (optional) or automatically build the initial firmware with the bootloader.
	``$ cd tools``
	``$ python bl_build.py --initial-firmware <firmware>``
 - Protect a firmware
   ``$ python fw_protect.py --infile [infile] --outfile [outfile] --version [version] --message [message]``

### Functions
Bootloader.c
 - init_interfaces() prepares the bootloader for communication
 - load_metadata() receives metadata and writes it to fw_update. This function also conducts boundchecks and prevents rollbacks. 
 - load_firmware() receives the firmware and verifies it
 - decrypt_and_write_firmware() uses AES to decrypt the firmware and write it to flash memory 

## Design Implementations
AES-CBC-256  
ECDSA with P-256 curve  
Frame size is 256
