# Linux-boot-from-SD-card test for the Arty Z7-10.
#
# Independent of the top-level bare-metal `all`/`run` flow in sw/ -- this
# builds a full FSBL + U-Boot + Linux kernel + BusyBox-initramfs boot chain
# from source (no PetaLinux) and lays it out on an SD card. See
# ../AGENTS.md and ../README.md for the physical steps (JP4 jumper, UART).

XSA          := ../arty-z7-soc/hw/arty_z7_soc.xsa

VITIS        := /home/per/tools/Xilinx/2026.1/2026.1/Vitis/bin/vitis
BOOTGEN      := /home/per/tools/Xilinx/2026.1/2026.1/Vitis/bin/bootgen
ARMHF_GCC_DIR:= /home/per/tools/Xilinx/2026.1/2026.1/gnu/aarch32/lin/gcc-arm-linux-gnueabi/bin
CROSS_COMPILE:= arm-linux-gnueabihf-

# xlsclients is an X11 utility that Vitis's headless tools probe for even
# in batch mode; it is not packaged on this system, so a no-op shim is
# added ahead of it on PATH (see AGENTS.md "Critical Xilinx Tooling Notes").
XLSCLIENTS_SHIM := $(CURDIR)/.xlsclients-shim
export PATH := $(XLSCLIENTS_SHIM):$(ARMHF_GCC_DIR):$(PATH)

FSBL_ELF     := fsbl/fsbl.elf
UBOOT_ELF    := u-boot/u-boot.elf
KERNEL_ZIMAGE:= kernel/src/arch/arm/boot/zImage
KERNEL_DTB   := kernel/src/arch/arm/boot/dts/xilinx/zynq-arty-z7.dtb
RAMDISK      := rootfs/uramdisk.image.gz
BOOTBIN      := boot/BOOT.BIN
BOOTSCR      := boot/boot.scr

.PHONY: all
all: $(FSBL_ELF) $(UBOOT_ELF) $(KERNEL_ZIMAGE) $(RAMDISK) $(BOOTBIN) $(BOOTSCR)

$(XLSCLIENTS_SHIM):
	mkdir -p $(XLSCLIENTS_SHIM)
	printf '#!/bin/sh\nexit 0\n' > $(XLSCLIENTS_SHIM)/xlsclients
	chmod +x $(XLSCLIENTS_SHIM)/xlsclients

# --- FSBL ---------------------------------------------------------------
.PHONY: fsbl
fsbl: $(FSBL_ELF)

$(FSBL_ELF): $(XSA) fsbl/build_fsbl.py $(XLSCLIENTS_SHIM)
	cd fsbl && $(VITIS) -s build_fsbl.py

# --- U-Boot ---------------------------------------------------------------
.PHONY: uboot
uboot: $(UBOOT_ELF)

u-boot/src:
	git clone --depth 1 https://github.com/Xilinx/u-boot-xlnx.git u-boot/src
	cp u-boot/zynq-arty-z7.dts u-boot/src/arch/arm/dts/zynq-arty-z7.dts
	cd u-boot/src && \
	  sed -i '/dtb-\$$(CONFIG_ARCH_ZYNQ) += \\/a\\tzynq-arty-z7.dtb \\' arch/arm/dts/Makefile

$(UBOOT_ELF): u-boot/src
	cd u-boot/src && \
	  $(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) xilinx_zynq_virt_defconfig && \
	  ./scripts/config --set-str DEFAULT_DEVICE_TREE zynq-arty-z7 && \
	  $(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) olddefconfig && \
	  $(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) DEVICE_TREE=zynq-arty-z7 -j$$(nproc)
	cp u-boot/src/u-boot.elf $(UBOOT_ELF)

# --- Kernel ---------------------------------------------------------------
.PHONY: kernel
kernel: $(KERNEL_ZIMAGE)

kernel/src:
	git clone --depth 1 https://github.com/Xilinx/linux-xlnx.git kernel/src
	cp kernel/zynq-arty-z7.dts kernel/src/arch/arm/boot/dts/xilinx/zynq-arty-z7.dts
	cd kernel/src && \
	  sed -i '/dtb-\$$(CONFIG_ARCH_ZYNQ) += \\/a\\tzynq-arty-z7.dtb \\' arch/arm/boot/dts/xilinx/Makefile

$(KERNEL_ZIMAGE) $(KERNEL_DTB) &: kernel/src
	cd kernel/src && \
	  $(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) xilinx_zynq_defconfig && \
	  $(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) -j$$(nproc) zImage dtbs

# --- Rootfs (BusyBox initramfs) -------------------------------------------
.PHONY: rootfs
rootfs: $(RAMDISK)

rootfs/src:
	git clone --depth 1 https://git.busybox.net/busybox rootfs/src
	cd rootfs/src && $(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) defconfig
	sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/' rootfs/src/.config
	cd rootfs/src && yes "" | $(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) oldconfig

rootfs/src/busybox: rootfs/src
	cd rootfs/src && $(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) -j$$(nproc)

$(RAMDISK): rootfs/src/busybox $(UBOOT_ELF)
	rm -rf rootfs/initramfs
	mkdir -p rootfs/initramfs/{proc,sys,dev,tmp,root,mnt}
	cd rootfs/src && $(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) \
	  CONFIG_PREFIX=$(CURDIR)/rootfs/initramfs install
	cp rootfs/init rootfs/initramfs/init
	chmod +x rootfs/initramfs/init
	cd rootfs/initramfs && \
	  find . -print0 | bsdtar --null -cf - --format=newc -T - | gzip -9 > ../rootfs.cpio.gz
	u-boot/src/tools/mkimage -A arm -O linux -T ramdisk -C gzip \
	  -d rootfs/rootfs.cpio.gz $(RAMDISK)

# --- BOOT.BIN + boot.scr ---------------------------------------------------
.PHONY: boot
boot: $(BOOTBIN) $(BOOTSCR)

$(BOOTBIN): $(FSBL_ELF) $(UBOOT_ELF) boot/boot.bif
	cd boot && $(BOOTGEN) -image boot.bif -arch zynq -o BOOT.BIN -w on

$(BOOTSCR): boot/boot.cmd u-boot/src
	u-boot/src/tools/mkimage -A arm -O linux -T script -C none -d boot/boot.cmd $(BOOTSCR)

# --- SD card layout ---------------------------------------------------------
# Copies boot artifacts onto an already-formatted, already-mounted FAT32
# partition. Deliberately takes a mountpoint (SDMNT), not a raw device node,
# so this target can never itself partition/format/overwrite a whole disk --
# that destructive step is left to the user to do explicitly beforehand.
.PHONY: sdcard
sdcard: all
ifndef SDMNT
	$(error Usage: make sdcard SDMNT=/path/to/mounted/fat32/partition)
endif
	cp $(BOOTBIN) $(SDMNT)/BOOT.BIN
	cp $(KERNEL_DTB) $(SDMNT)/devicetree.dtb
	cp $(KERNEL_ZIMAGE) $(SDMNT)/zImage
	cp $(BOOTSCR) $(SDMNT)/boot.scr
	cp $(RAMDISK) $(SDMNT)/uramdisk.image.gz
	sync
	@echo "SD card contents written to $(SDMNT). Safe to unmount and eject."

.PHONY: clean
clean:
	rm -rf fsbl/vitis_ws fsbl/fsbl.elf
	rm -rf u-boot/src u-boot/u-boot.elf
	rm -rf kernel/src
	rm -rf rootfs/src rootfs/initramfs rootfs/rootfs.cpio.gz rootfs/uramdisk.image.gz
	rm -rf boot/BOOT.BIN boot/boot.scr
	rm -rf $(XLSCLIENTS_SHIM)

.PHONY: help
help:
	@echo "Usage: make [target]"
	@echo "  all    - Build FSBL, U-Boot, kernel, rootfs, and BOOT.BIN/boot.scr"
	@echo "  sdcard SDMNT=<path> - Copy boot artifacts onto a mounted FAT32 SD partition"
	@echo "  clean  - Remove all cloned sources and build artifacts"
