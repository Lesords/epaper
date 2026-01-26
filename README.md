# epaper

## Dependencies

- TI Processor SDK Linux
- aarch64 cross compiler (for example: aarch64-linux-gnu-gcc)

## Compilation

You need to modify the sysroot variable in the Makefile to the location of the SDK, for example
```bash
SYSROOT ?= /home/lese/ti-processor-sdk-linux-am62lxx-evm-11.01.16.13/linux-devkit/sysroots/aarch64-oe-linux
```

Then you can compile the code with
```bash
make
```

## Usage

The compiled program is named epaper_test. You can run this program directly to get help.
```bash
epaper_test

# output
Usage: ./epaper_test [boy girl beaglebone tower eagle_binary eagle_bayer eagle_atkinson --fast --clear]
```

Usage examples
```bash
# Slow Mode
epaper_test beaglebone
epaper_test eagle_binary

# Fast Mode
# You need to refresh slowly once before refreshing quickly
epaper_test beaglebone
epaper_test eagle_binary --fast

# Or use clear parameters in fast mode
epaper_test eagle_binary --fast --clear

# Test Mode
epaper_test test
epaper_test test --full
```
