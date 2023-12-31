
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
  - Start an update
   ``$ python fw_update --firmware [firmware] ``

## Design Implementations
### Notable Functions
 - ``init_interfaces()`` prepares the bootloader for communication. UART interfaces are setup and the initial firmware is loaded at this stage.
 - ``load_metadata()`` parses the metadata received from the update tool into an internal structure to be used throughout the program. Sanity checks are conducted to ensure the data received is acceptable.
 - ``load_firmware()`` initalizes communication with the update tool before writing encrypted firmware to a buffer.
 - ``decrypt_and_write_firmware()`` uses AES-256 to decrypt the firmware downloaded from ``load_firmware`` and writes it to memory.

### Notable Information

 - The bootloader is designed to support an message size up to 1kB (1,000 bytes) and a firmware size up to 30kB (30,000 bytes), along with a maximum version of 65,535.
 - Firmware data is sent in chunks of 256 bytes
 - Any modification of the firmware file will cancel the installation and reset the device.
 - You can ignore ``caller.py`` and ``uart.py``. We needed these Python scripts for our ``.vscode`` tasks (made our lives 10x easier).

we love bwsi!

