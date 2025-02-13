# Monix Boot Protocol

This document outlines the monix kernel boot protocol, covering how the kernel expects to be loaded by the bootloader, and the expected processor state. 

### Bootloader

The bootloader is expected to load both the kernel and device tree into memory at 2MB boundaries, as close to the base of physical memory as possible. The device tree should directly follow the kernel image.

A boot arguments structure should then be prepared. This will outline the system memory properties (virtual/physical base address, memory size) as determined by the bootloader, the physical base address and size of the kernel image and device tree, bootargs version, flags and bootloader version string.

> Note: The UART base is passed via the bootargs, but this planned to only be a debug option in the future, and the kernel will discover the UART from the device tree 

This section complies with version 1.1 of the bootargs (`BOOT_ARGS_VERSION_1_1`).

### Processor State

The kernel must enter at EL2. Support for entering at EL1 will be added in the future. The physical address of the boot arguments should be placed in register `x0`, with the remaining `x1-x3` being zero'd. The following CPU state applies:
* Interrupts should be disabled
* The MMU must be disabled at EL2 (`SCTLR_EL2` should be `res1`)
* GIC system register access must be enabled at all exception levels
    * `ICC_SRE_ELx`

There are no requirements for the remaining system registers. It is preferable that any EL2 or EL1 configuration bits are zero, as the kernel currently targets ARMv8.0.

## Kernel startup

Kernel startup is split into the Reset Vector and Entry Point. Secondary CPUs are handled seperately.

The reset vector executes at EL2. It is responsible for configuring an early exception vector, so exception aren't taken to the firmware, and programming the relevant system registers to switch to non-secure EL1. Just before the kernel switches to EL1, the CPU state is as follows:
* `VBAR_EL1` is set to the address of `_LowExceptionVectorBase`
* `SCTLR_EL1` is `res1`
* `HCR_EL2` configures EL1 to be AArch64
* `SPSR_EL2` enables EL1 and SError and External Aborts
* `ICC_SRE_EL2` enables GICv3 system registers at EL2
* `ELR_EL2` is set to the address of `_start`

The main kernel startup flow, now executing at EL1, will configure the stack pointer, read the physical/virtual bases and memory size from the boot arguments, configure initial page tables, enable the MMU, re-configure the exception vector to the main kernel one, unmask interrupts and calculate the kernel virtual address of the C entry point.

Two page tables are created: an identity table, and a kernel virtual address (KVA) table. The identity table maps a physical address to the same virtual address, so the kernel can continue to execute the next address in the PC once the MMU is enabled. The KVA table is temporary until virtual memory is configured, and allows the kernel to enter with a kernel virtual address.

The virtual memory configuration is detailed more in it's own document, but the summary of what's configured in `TCR_EL1` is:
* 4KB granule size
* 40-bit virtual addresses
* TBI disabled

### Secondary CPUs

Monix currently doesn't support secondary CPUs, although this is planned. It is expected that the bootloader will hold the secondary CPUs in a low-power state until they are woken up via PSCI, and once this has occured they should enter the kernel at `_SecondaryResetVector`. Handling of secondary CPUs will be detailed in a seperate document.