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

# old docs
## running the insecure example

1. build the firmware by navigating to `firmware/firmware`, and running `make`.
2. build the bootloader by navigating to `tools`, and running `python bl_build.py`

## troubleshooting

Ensure that BearSSL is compiled for the stellaris: `cd ~/lib/BearSSL && make CONF=../../stellaris/bearssl/stellaris clean && make CONF=../../stellaris/bearssl/stellaris`
