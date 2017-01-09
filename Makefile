MAKEFILE = Makefile.rpi-boot
MAKEFILE_IN = $(MAKEFILE).in

# Allow cross as well as native compilation
CROSS_COMPILE  ?=
ARMCC           = $(CROSS_COMPILE)gcc

all: kernel.img

.PHONY: clean kernel.img libfs.a qemu qemu-gdb dump kernel-qemu.elf kernel.img-atag

$(MAKEFILE): $(MAKEFILE_IN) config.h Makefile
	$(ARMCC) -P -traditional-cpp -std=gnu99 -E -o $(MAKEFILE) -x c $(MAKEFILE_IN)

clean: $(MAKEFILE)
	$(MAKE) -f $(MAKEFILE) clean

kernel.img: $(MAKEFILE)
	$(MAKE) -f $(MAKEFILE) kernel.img

kernel.img-atag: $(MAKEFILE)
	$(MAKE) -f $(MAKEFILE) kernel.img-atag

libfs.a: $(MAKEFILE)
	$(MAKE) -f $(MAKEFILE) libfs.a

kernel-qemu.elf: $(MAKEFILE)
	$(MAKE) -f $(MAKEFILE) kernel-qemu.elf

qemu: $(MAKEFILE)
	$(MAKE) -f $(MAKEFILE) qemu

qemu-gdb: $(MAKEFILE)
	$(MAKE) -f $(MAKEFILE) qemu-gdb

dump: $(MAKEFILE)
	$(MAKE) -f $(MAKEFILE) dump

raspbootin-server: raspbootin-server.c crc32.c
	$(CC) -g -std=c99 -o $@ $^
