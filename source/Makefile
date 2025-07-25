# Directories
SRC_DIR = source

# Source files
C_SRC := $(shell find $(SRC_DIR) -type f -name '*.c')
LINKER_SCRIPT = $(GNUEFI_DIR)/gnuefi/elf_$(ARCH)_efi.lds

# Object files
C_OBJ := $(patsubst $(SRC_DIR)/%.c, $(SRC_BUILD_DIR)/%.o, $(C_SRC))
GNUEFI_OBJ = $(GNUEFI_BUILD_DIR)/gnuefi/crt0-efi-$(ARCH).o

# Preprocessor
CDEFS =											\
	-D DB_VERSION="\"$(DBOOT_VERSION)\""		\

# Flags
CFLAGS =						\
	-Wall -Wextra				\
	-Wno-pointer-sign			\
	-Wno-error=pragmas			\
	-std=c11					\
	-fPIE						\
	-ffreestanding				\
	-fno-strict-aliasing		\
	-fno-stack-protector		\
	-fno-stack-check			\
	-fno-merge-all-constants	\
	-funsigned-char				\
	-fshort-wchar				\
	-mno-red-zone				\
	-mno-avx					\
	-DEFI_FUNCTION_WRAPPER		\
	-DGNU_EFI_USE_MS_ABI		\
	-I $(SRC_DIR)

LDFLAGS = 						\
	-nostdlib 					\
	-z nocombreloc				\
	-shared 					\
	-Bsymbolic 					\
	--warn-common				\
	--no-undefined				\
	--fatal-warnings			\
	-I $(SRC_DIR)				\
	-T $(LINKER_SCRIPT)

# Add GNU-EFI
CFLAGS +=										\
	-I $(GNUEFI_DIR)/inc						\
	-I $(GNUEFI_DIR)/inc/$(ARCH)				\
	-I $(GNUEFI_DIR)/inc/protocol				\

LDFLAGS +=										\
	-L $(GNUEFI_BUILD_DIR)/lib					\
	-L $(GNUEFI_BUILD_DIR)/gnuefi

LIBS += -lefi -lgnuefi

# Setup architecture flags
ifeq ($(ARCH), x86_64)
	CFLAGS += -DCONFIG_x86_64
endif

# Compiler optimization flags
ifeq ($(OPTIMIZE), true)
	CFLAGS += -Os
	LDFLAGS += -Os
endif

# Debug mode
ifeq ($(DEBUG), true)
	CDEFS += -D DB_DEBUG
endif

# Build C binaries
$(SRC_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(SRC_BUILD_DIR)
	@ mkdir -p $(dir $@)
	@ echo -e $(MSG_COMP_CC) $<
	@ $(CC) $(CFLAGS) $(CDEFS) -c $^ -o $@

# Build EFI shared object
$(DBOOT_SO): $(C_OBJ) $(GNUEFI_OBJ) | $(BUILD_DIR)
	@ $(LD) $(LDFLAGS) $^ -o $@ $(LIBS)
	@ echo -e $(MSG_EXEC_LD) Linked $@
	@ echo -e $(MSG_DONE_EFI_OBJ)