CC = gcc
LD = ld
OBJCOPY = objcopy
QEMU = qemu-system-$(ARCH)

# Build directories
BUILD_DIR = build
SRC_BUILD_DIR = build/source

# GNU-EFI directories
GNUEFI_DIR = gnuefi
GNUEFI_BUILD_DIR = gnuefi/$(ARCH)

# Output files
DBOOT_SO = $(SRC_BUILD_DIR)/dboot.so
DBOOT_EFI = $(BUILD_DIR)/dboot.efi
DBOOT_IMG = $(BUILD_DIR)/dboot.img

# Makefile targets
all: lib-gnuefi dboot-image
dboot-efi: $(DBOOT_EFI)
dboot-image: $(DBOOT_IMG)

# Ensure build directories exist
$(BUILD_DIR) $(SRC_BUILD_DIR):
	@ mkdir -p $@

# Build GNU-EFI
lib-gnuefi:
	@ echo -e $(MSG_COMP_LIB)" GNU-EFI"
	@ make -C $(GNUEFI_DIR) gnuefi $(SUPPRESS)
	@ echo -e $(MSG_DONE_GNUEFI)

# Build the bootloader EFI
OBJCOPY_FLAGS := -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc
$(DBOOT_EFI): lib-gnuefi $(DBOOT_SO) | $(BUILD_DIR)
	@ $(OBJCOPY) $(OBJCOPY_FLAGS) --target efi-app-$(ARCH) $(DBOOT_SO) $@
	@ echo -e $(MSG_DONE_EFI)$@

# Build the bootloader image
$(DBOOT_IMG): $(DBOOT_EFI) | $(BUILD_DIR)
	@ command -v mtools > /dev/null 2>&1 || {					\
		echo -e $(RED)"Error: 'mtools' could not be found.";	\
		echo -e "\tPlease install it to continue."$(RESET);		\
		exit 1;													\
	}

	@ truncate -s $(IMG_SIZE) $@
	@ mformat -i $@ -F -v UEFI_BOOT ::

	@ mmd -i $@ ::/EFI
	@ mmd -i $@ ::/EFI/BOOT
	@ mcopy -i $@ $^ ::/EFI/BOOT/BOOT$(EFI_ARCH).EFI
	@ mcopy -i $@ examples/advanced/dboot.conf ::/EFI/DBOOT.CONF

	@ echo -e $(MSG_DONE_IMG)$@

# Run QEMU in normal mode
qemu-run: $(DBOOT_IMG)
	@ echo -e $(MSG_QEMU_RUN)
	@ $(QEMU) -machine accel=kvm -m 1024						\
		-cpu host -bios $(OVMF_FIRM_PATH)						\
		-drive format=raw,unit=0,file=$^						\
		-net none

# Run QEMU in debug mode
qemu-debug: $(DBOOT_IMG)
	@ echo -e $(MSG_QEMU_RUN)" in debug mode"
	@ $(QEMU) -machine accel=kvm -m 1024M						\
		-cpu host -bios $(OVMF_FIRM_PATH)						\
		-drive format=raw,unit=0,file=$^						\
		-net none -s -S

# Generate compile_commands.json
gen-cc:
	@ command -v bear > /dev/null 2>&1 || {						\
		echo -e $(RED)"Error: 'bear' could not be found.";		\
		echo -e "\tPlease install it to continue."$(RESET);		\
		exit 1;													\
	}
	@ bear -- make -B all
	@ echo -e $(MSG_GEN_CC_DONE)

# Clean build directory
clean:
	@ rm -rf $(BUILD_DIR)
	@ echo -e $(MSG_CLEAN)" build files"

# Clean compile_commands.json
clean-cc:
	@ rm -f compile_commands.json
	@ echo -e $(MSG_CLEAN)" compile_commands.json"

# Clean GNU-EFI
clean-gnuefi:
	@ make -C gnuefi clean $(SUPPRESS)
	@ echo -e $(MSG_CLEAN)" GNU-EFI"

clean-all: clean clean-cc clean-gnuefi

# Include other makefiles
include make/utils.mk
include make/config.mk

include source/Makefile

# Phony targets
.PHONY: all dboot-efi dboot-image lib-gnuefi qemu-run qemu-debug gen-cc clean clean-cc clean-genefi clean-all help
