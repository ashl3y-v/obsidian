# OBSIDIAN

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
## Design Implementations



