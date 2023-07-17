# obsidian
Team Obsidian - BWSI Design Challenge 2023
Members: Anna, Ashley, Noah, Riley

# goals
* Bootloader
* Host tools
  * Bootloader - build tool - Noah
  * Bootloader - emulate tool - Riley
  * Firmware - bundle and protect tool - Ashley
  * Firmware - update tool - Anna

# to do

# build tool
## run
./bl_build --initial-firmware <firmware_binary>

# emulate tool
## run
./bl_emulate --boot-path <bootloader_binary> [--debug]

# bundle and protect tool
## run
./fw_protect --infile <unprotected_firmware_filename> --version <version_number> --message <release_message> --outfile <protected_firmware_output_filename>

# update tool 
## run
./fw_update --port <serial port> --firmware <filename_of_protected_firmware>

# old docs
## running the insecure example

1. build the firmware by navigating to `firmware/firmware`, and running `make`.
2. build the bootloader by navigating to `tools`, and running `python bl_build.py`

## troubleshooting

Ensure that BearSSL is compiled for the stellaris: `cd ~/lib/BearSSL && make CONF=../../stellaris/bearssl/stellaris clean && make CONF=../../stellaris/bearssl/stellaris`
