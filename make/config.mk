# Version info
DBOOT_VERSION = 0.0.0

# Set target architecture
ARCH = x86_64
EFI_ARCH = X64

# Toggle command output supression
SUPPRESS = >/dev/null 2>/dev/null

# Build options
OPTIMIZE = false
DEBUG = true

# QEMU options
IMG_SIZE = 200M
OVMF_FIRM_PATH = /usr/share/ovmf/x64/OVMF.4m.fd
HOST_VMLINUZ_PATH = /boot/vmlinuz-linux
HOST_INITRAMFS_PATH = /boot/initramfs-linux.img
