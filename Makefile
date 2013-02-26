ARMCC ?= arm-none-eabi-gcc
ARMLD ?= arm-none-eabi-ld
ARMOBJCOPY ?= arm-none-eabi-objcopy
QEMU ?= qemu-system-arm

all: kernel.img

CFLAGS += -pedantic -pedantic-errors -nostdlib -nostartfiles -ffreestanding -Wall -Wextra -Werror -Wshadow
CFLAGS += -std=gnu99
CFLAGS += -O0
CFLAGS += -I.
CFLAGS += -g
CFLAGS += -DDEBUG

QEMUFLAGS = -cpu arm1176 -m 256 -M raspi -serial stdio -kernel kernel-qemu.img -usb
SDFLAGS = -sd sd.img

OBJS = main.o boot.o uart.o stdio.o stream.o atag.o mbox.o fb.o stdlib.o font.o console.o mmio.o heap.o malloc.o printf.o emmc.o block.o mbr.o fat.o vfs.o multiboot.o memchunk.o ext2.o elf.o usb.o timer.o util.o

.PHONY: clean
.PHONY: qemu
.PHONY: qemu-gdb

kernel.elf: $(OBJS) linker.ld
	$(ARMCC) -nostdlib $(OBJS) -Wl,-T,linker.ld -o $@ -lgcc

kernel.img: kernel.elf
	$(ARMOBJCOPY) kernel.elf -O binary kernel.img

kernel-qemu.elf: $(OBJS) linker-qemu.ld
	$(ARMCC) -nostdlib $(OBJS) -Wl,-T,linker-qemu.ld -o $@ -lgcc

kernel-qemu.img: kernel-qemu.elf
	$(ARMOBJCOPY) kernel-qemu.elf -O binary kernel-qemu.img

clean:
	$(RM) -f $(OBJS) kernel.elf kernel.img kernel-qemu.img kernel-qemu.elf

%.o: %.c Makefile
	$(ARMCC) $(CFLAGS) -c $< -o $@

%.o: %.s Makefile
	$(ARMCC) $(ASFLAGS) -c $< -o $@

qemu: kernel-qemu.img
	if [ -f sd.img ]; then \
		$(QEMU) $(QEMUFLAGS) -sd sd.img; \
	else \
		$(QEMU) $(QEMUFLAGS); \
	fi

qemu-gdb: kernel-qemu.img
	if [ -f sd.img ]; then \
		$(QEMU) $(QEMUFLAGS) -sd sd.img -s -S; \
	else \
		$(QEMU) $(QEMUFLAGS) -s -S; \
	fi

