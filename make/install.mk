install: all
	@ $(call check_defined, DISK, Please define install disk)
	@ $(call check_defined, PART, Please define install partition)

# Create boot entry if it does not already exist
	@ $(eval ENTRY_EXISTS := $(shell efibootmgr | grep "$(EFI_ENTRY_NAME)"))
	@ if [ -z "$(ENTRY_EXISTS)" ]; then            \
		echo -e $(MSG_INST_ENTRY);                 \
		sudo efibootmgr -c                         \
			-d "$(DISK)" -p "$(PART)"              \
			-L "$(EFI_ENTRY_NAME)"                 \
			-l "$(EFI_ENTRY_PATH)"                 \
			$(SUPPRESS);                           \
	else                                           \
		echo -e $(WRN_INST_ENTRYEXISTS);           \
	fi

# Install DBOOT EFI application
	@ echo -e $(MSG_INST_EFI)
	@ sudo install -d /boot/EFI/DBOOT
	@ sudo install -m 644 $(DBOOT_EFI) /boot/EFI/DBOOT/BOOTX64.EFI

	@ echo -e $(MSG_INST_DONE)

uninstall:
# Get DBOOT EFI entry bootnum
	@ echo -e $(MSG_UNINST_FINDENTRY)
	@ $(eval BOOTNUM := $(shell efibootmgr | grep "$(EFI_ENTRY_NAME)" | awk '{print $$1}' | sed 's/Boot//;s/\*//'))
	@ $(call check_defined, BOOTNUM, Failed to find DBOOT entry)

# Delete DBOOT EFI entry
	@ sudo efibootmgr -b "$(BOOTNUM)" -B $(SUPPRESS)
	@ echo -e $(MSG_UNINST_ENTRY)

# Remove DBOOT EFI directory
	@ echo -e $(MSG_UNINST_EFI)
	@ sudo rm -rf /boot/EFI/DBOOT

	@ echo -e $(MSG_UNINST_DONE)
