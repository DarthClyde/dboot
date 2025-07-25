# Colors for output
CYAN="\e[1;36m"
GREEN="\e[1;32m"
RED="\e[1;31m"
YELLOW="\e[1;33m"
PURPLE="\e[1;35m"
WHITE="\e[1;37m"
RESET="\e[0m"

# Prefixs
PFX_COMP = $(CYAN)"[COMP]    "$(RESET)
PFX_DONE = $(GREEN)"[DONE]    "$(RESET)
PFX_EXEC = $(PURPLE)"[EXEC]    "$(RESET)
PFX_ERRO = $(RED)"[ERRO]    "$(RESET)
PFX_WARN = $(YELLOW)"[WARN]    "$(RESET)

# Compiling messages
MSG_COMP_ASM = $(PFX_COMP)$(WHITE)"ASM   "$(RESET)
MSG_COMP_CC = $(PFX_COMP)$(WHITE)"CC    "$(RESET)
MSG_EXEC_LD = $(PFX_EXEC)$(WHITE)"LD    "$(RESET)
MSG_COMP_LIB = $(PFX_COMP)$(WHITE)"LIB   "$(RESET)

MSG_DONE_GNUEFI = $(PFX_DONE)"Finished compiling GNU-EFI"
MSG_DONE_EFI_OBJ = $(PFX_DONE)"Finished compiling EFI object"
MSG_DONE_EFI = $(PFX_DONE)"Created bootloader EFI: "
MSG_DONE_IMG = $(PFX_DONE)"Created bootloader image: "

# Other messages
MSG_CLEAN = $(PFX_DONE)"Finished cleaning"
MSG_QEMU_PREP = $(PFX_EXEC)"Setting up disk structure for QEMU"
MSG_QEMU_RUN = $(PFX_EXEC)"Running QEMU"
MSG_GEN_CC_DONE = $(PFX_DONE)"Generated 'compile_commands.json' with 'bear'"

# Help menu
help:
	@ echo "Available Targets:"
	@ echo "  all           - Build the bootloader EFI + libraries"
	@ echo "  dboot-efi     - Build the bootloader EFI application"
	@ echo ""
	@ echo "  lib-gnuefi    - Build the 'GNU-EFI' library"
	@ echo ""
	@ echo "  clean         - Clean the build directory"
	@ echo "  clean-cc      - Clean the compile_commands.json file"
	@ echo "  clean-gnuefi  - Clean the 'GNU-EFI' build directory"
	@ echo "  clean-all     - Runs: clean, clean-cc, clean-gnuefi"
	@ echo ""
	@ echo "  qemu-prep      - Prepare the QEMU FAT32 image file"
	@ echo "  qemu-run      - Run QEMU normally"
	@ echo "  qemu-debug    - Run QEMU with GDB debugging enabled"
	@ echo ""
	@ echo "  gen-cc        - Generate compile_commands.json file. Runs 'all'"
	@ echo "  help	        - Display this help message"
