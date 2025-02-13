# Monix Kernel

Monix kernel source code repository.

Monix is a personal hobby project operating system kernel which begun in May 2023 as a simple bare-metal Raspberry Pi kernel, and has since refocused to be a generic AArch64 operating system. Monix is not intended for any real-world practical purpose, and is provided entirely for educational purposes. 

Monix is intended to be loaded by the tinyBoot bootloader. This is a closed-source (currently) two-stage bootloader, with binaries for QEMU provided as download under the Releases tab of this repository.

Technical documentation for monix is found under the `docs/` directory, and contribution guidelines can be found in `CONTRIBUTING`. Monix is released under a GPLv3 license.

While you are free to contribute to Monix, it is encouraged that you upstream all your changes to this repository.

## Booting

The current version of Monix contains a device tree for a QEMU configuration called "tiny-ex1". This is the base QEMU device tree with the `virtio_mmio` devices removed for simplicity. The necessary tinyBoot binaries can be found under the releases tab, and should be sufficient to boot Monix 0.0.2.

A script, `boot.py`, is provided with Monix to boot the Kernel. Set the following environment variables and run the script:
```
$ python3 scripts/boot.py -r {$TINYROM_IMG} -f {$TBOOT_FIRMWARE},0x48000000 {$KERNEL_IMG},0x49000000 -t {$DTREE} -q $QEMU
```
Preferred version of QEMU is 8.2.0.

## Source Structure

The current Monix source structure looks as follows:

| Directory | Description |
| --------- | ----------- |
| arch      | aarch64 architecture-specific code
| docs      | documentation
| drivers   | monix device drivers
| kern      | core kernel code
| libkern   | kernel support (linked lists, logging, utils), libfdt, tinylibc
| platform  | platform interface
| scripts   | general scripts
