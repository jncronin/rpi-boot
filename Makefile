ARM-CC ?= arm-none-eabi-gcc
ARM-LD ?= arm-none-eabi-ld
ARM-OBJCOPY ?= arm-none-eabi-objcopy
QEMU ?= qemu-system-arm

all: kernel.img

CFLAGS := -pedantic -pedantic-errors -nostdlib -nostartfiles -ffreestanding -Wall -Wextra -Werror
CFLAGS += -std=gnu99
CFLAGS += -O0
CFLAGS += -I.
CFLAGS += -g

QEMU-FLAGS = -cpu arm1176 -m 256 -M raspi -serial stdio -kernel kernel-qemu.img
SD-FLAGS = -sd sd.img

OBJS = main.o boot.o uart.o stdio.o stream.o atag.o mbox.o fb.o stdlib.o font.o console.o mmio.o heap.o malloc.o printf.o emmc.o block.o mbr.o fat.o vfs.o multiboot.o memchunk.o

.PHONY: clean
.PHONY: qemu
.PHONY: qemu-gdb

kernel.elf: $(OBJS) linker.ld
	$(ARM-CC) -nostdlib $(OBJS) -Wl,-T,linker.ld -o $@ -lgcc

kernel.img: kernel.elf
	$(ARM-OBJCOPY) kernel.elf -O binary kernel.img

kernel-qemu.elf: $(OBJS) linker-qemu.ld
	$(ARM-CC) -nostdlib $(OBJS) -Wl,-T,linker-qemu.ld -o $@ -lgcc

kernel-qemu.img: kernel-qemu.elf
	$(ARM-OBJCOPY) kernel-qemu.elf -O binary kernel-qemu.img

clean:
	$(RM) -f $(OBJS) kernel.elf kernel.img kernel-qemu.img kernel-qemu.elf

%.o: %.c Makefile
	$(ARM-CC) $(CFLAGS) -c $< -o $@

%.o: %.s Makefile
	$(ARM-CC) $(ASFLAGS) -c $< -o $@

qemu: kernel-qemu.img
	if [ -f sd.img ]; then \
		$(QEMU) $(QEMU-FLAGS) -sd sd.img; \
	else \
		$(QEMU) $(QEMU-FLAGS); \
	fi

qemu-gdb: kernel-qemu.img
	if [ -f sd.img ]; then \
		$(QEMU) $(QEMU-FLAGS) -sd sd.img -s -S; \
	else \
		$(QEMU) $(QEMU-FLAGS) -s -S; \
	fi

