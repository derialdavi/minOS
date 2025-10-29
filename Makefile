CC = /usr/bin/clang
CFLAGS = -std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib
LD_SCRIPT = kernel.ld
LDFLAGS = -T$(LD_SCRIPT) -Wl,-Map=kernel.map

SRC = kernel.c common.c
OUT = kernel.elf

all: $(OUT)

$(OUT): $(SRC) $(LD_SCRIPT)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $@

clean:
	rm -rf $(OUT) kernel.map

run: $(OUT)
	qemu-system-riscv32 \
		-machine virt \
		-bios default \
		-nographic \
		-serial mon:stdio \
		--no-reboot \
    	-kernel $(OUT)