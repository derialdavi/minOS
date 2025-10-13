#!/bin/bash
set -xue

# QEMU file path
QEMU=qemu-system-riscv32

# Path to compiler and flags
CC=/usr/bin/clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib"

# Build the kernel
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
    kernel.c common.c

# Start QEMU
# Initialize a virtual machine with OpenSBI as firmware, no GUI, standard I/O connected to
# the virtual machine serial port with possibility to switch from OS CLI to QEMU monitor and
# doesn't reboot in case the virtual machine crashes
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
    -kernel kernel.elf