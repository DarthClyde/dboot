#include "protos/linux.h"

#include <efilib.h>

#include "linux_bootparams.h"
#include "fs/fs.h"

#ifdef __x86_64__
typedef VOID (*handover_func)(VOID*, EFI_SYSTEM_TABLE*, struct boot_params*);

static inline VOID linux_efi_handover(struct boot_params* params)
{
	handover_func handover;

	// Disable interupts
	__asm__ VOLATILE("cli");

	// Calculate handover address
	UINTN handover_addr = params->hdr.code32_start + 512 + params->hdr.handover_offset;
	Print(L"\n[linux] Calling EFI handover at 0x%lx...\n", handover_addr);

	// Perform handover
	handover = (handover_func)(handover_addr);
	handover(fs_get_image(), ST, params);
}
#endif

static inline EFI_STATUS validate_setup_header(struct setup_header* setup)
{
	// Ensure EFI handover is supported
	if (setup->version < 0x20B)
	{
		Print(L"[linux] Kernel version does not support EFI handover\n");
		return EFI_INCOMPATIBLE_VERSION;
	}
	if (setup->handover_offset == 0)
	{
		Print(L"[linux] Kernel does not support EFI handover\n");
		return EFI_UNSUPPORTED;
	}

	// Check that kernel is relocatable
	if (setup->relocatable_kernel == 0)
	{
		Print(L"[linux] Kernel is not relocatable\n");
		return EFI_UNSUPPORTED;
	}

	return EFI_SUCCESS;
}

EFI_STATUS linux_boot(CHAR16* kernel_path, CHAR16* initrd_path, CHAR8* cmdline)
{
	EFI_STATUS status                 = EFIERR(99);
	UINT32 signature                  = 0;
	file_t* file                      = NULL;

	struct boot_params* boot_params   = NULL;
	struct setup_header* setup_header = NULL;
	UINTN setup_header_end            = 0;
	UINTN setup_size                  = 0;

	UINTN kernel_size                 = 0;
	UINTN kernel_code_size            = 0;
	EFI_PHYSICAL_ADDRESS kernel_addr  = 0x0;

	UINTN initrd_size                 = 0;
	EFI_PHYSICAL_ADDRESS initrd_addr  = 0x0;

	UINTN cmdline_len                 = 0;
	EFI_PHYSICAL_ADDRESS cmdline_addr = 0x0;

	// Open the kernel bzImage
	Print(L"Loading kernel: %s\n", kernel_path);
	status = fs_file_open(fs_get_image(), kernel_path, &file);
	if (EFI_ERROR(status)) goto kernel_load_err;

	// Validate kernel signature
	status = fs_file_setpos(file, SETUP_SIGNATURE_OFFSET);
	status += fs_file_readr(file, sizeof(UINT32), &signature);
	if (EFI_ERROR(status) || signature != SETUP_SIGNATURE)
	{
		Print(L"[linux] Invalid kernel signature: 0x%lx\n", signature);
		goto kernel_load_err;
	}

	// Get setup header size
	status = fs_file_setpos(file, SETUP_HEADER_OFFSET);
	status += fs_file_readr(file, 0x01, &setup_size);
	if (EFI_ERROR(status)) goto kernel_load_err;

	// Get setup header end
	status = fs_file_setpos(file, SETUP_SIGNATURE_OFFSET - 1);
	status += fs_file_readr(file, 0x01, &setup_header_end);
	if (EFI_ERROR(status)) goto kernel_load_err;

	// Calculate setup header sizes
	if (setup_size == 0) setup_size = 4;
	setup_size       = (setup_size * 512) + 512;
	setup_header_end = SETUP_SIGNATURE_OFFSET + setup_header_end;

	// Allocate boot params buffer
	boot_params = AllocateZeroPool(sizeof(struct boot_params));
	if (!boot_params) goto kernel_load_err;
	setup_header = &boot_params->hdr;

	// Read setup header
	status = fs_file_setpos(file, SETUP_HEADER_OFFSET);
	status += fs_file_readr(file, setup_header_end - SETUP_HEADER_OFFSET, setup_header);
	if (EFI_ERROR(status)) goto kernel_load_err;

	// Validate setup header
	status = validate_setup_header(setup_header);
	if (EFI_ERROR(status)) goto kernel_load_err;
	Print(L"[linux] Boot protocol version: %d.%d [0x%x]\n", (setup_header->version >> 8) & 0xFF,
	      setup_header->version & 0xFF, setup_header->version);

	// Get the kernel size
	status = fs_file_getsize(file, &kernel_size);
	if (EFI_ERROR(status)) goto kernel_load_err;

	// Configure setup headers
	setup_header->vid_mode       = 0xFFFF; /* Normal */
	setup_header->type_of_loader = 0xFF;   /* Undefined / Custom */
	setup_header->loadflags &= ~(1 << 5);  /* Early messages */

	// Set kernel alloc info
	kernel_addr      = 0x100000;
	kernel_code_size = kernel_size - setup_size;

	// Allocate kernel memory
	while (TRUE)
	{
		status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderData,
		                           EFI_SIZE_TO_PAGES(kernel_code_size), &kernel_addr);
		if (!EFI_ERROR(status)) break;

		if (kernel_addr == 0xFFF00000)
		{
			Print(L"Failed to allocate kernel memory\n");
			goto kernel_load_err;
		}

		kernel_addr += 0x100000;
	}
	Print(L"Allocated kernel memory at 0x%lx\n", kernel_addr);
	setup_header->code32_start = (UINT32)kernel_addr;

	// Read kernel code into allocated memory
	status = fs_file_setpos(file, setup_size);
	status += fs_file_readr(file, kernel_code_size, (VOID*)kernel_addr);
	if (EFI_ERROR(status)) goto kernel_load_err;
	fs_file_close(file);

	// Load cmdline
	if (cmdline)
	{
		// Set cmdline alloc info
		cmdline_len  = AsciiStrLen(cmdline);
		cmdline_addr = 0xA0000;

		// Allocate memory for cmdline
		status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress, EfiLoaderData,
		                           EFI_SIZE_TO_PAGES(cmdline_len + 1), &cmdline_addr);
		if (EFI_ERROR(status)) goto initrd_load_err;

		// Copy cmdline to allocated memory
		CopyMem((VOID*)cmdline_addr, cmdline, cmdline_len);
		((CHAR8*)cmdline_addr)[cmdline_len] = '\0';

		// Set boot params for cmdline
		setup_header->cmdline_ptr = (UINT32)cmdline_addr;
	}

	// Load initrd
	if (initrd_path)
	{
		// Open the initrd file
		Print(L"\nLoading initrd: %s\n", initrd_path);
		status = fs_file_open(fs_get_image(), initrd_path, &file);
		if (EFI_ERROR(status)) goto initrd_load_err;

		// Get the initrd size
		status = fs_file_getsize(file, &initrd_size);
		if (EFI_ERROR(status)) goto initrd_load_err;
		initrd_addr = 0x3FFFFFFF;

		// Allocate memory for initrd
		status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress, EfiLoaderData,
		                           EFI_SIZE_TO_PAGES(initrd_size), &initrd_addr);
		if (EFI_ERROR(status)) goto initrd_load_err;

		// Read initrd into allocate memory
		status = fs_file_readr(file, initrd_size, (VOID*)initrd_addr);
		if (EFI_ERROR(status)) goto initrd_load_err;
		fs_file_close(file);

		// Set boot params for initrd
		setup_header->ramdisk_image = (UINT32)initrd_addr;
		setup_header->ramdisk_size  = (UINT32)initrd_size;
#ifdef __x86_64__
		boot_params->ext_ramdisk_image = (UINT32)(initrd_addr >> 32);
		boot_params->ext_ramdisk_size  = (UINT32)(initrd_size >> 32);
#endif
	}

	// To Linux We Go :) !!
	linux_efi_handover(boot_params);

	//     "It was at this moment he knew... he f**ked up"
	// (╯°□°)╯ ┻━┻
	Print(L"[linux] EFI handover failed\n");
	status = EFI_LOAD_ERROR;
	goto end;

kernel_load_err:
	Print(L"Failed to load Linux kernel: %d\n", status);
	goto end;

initrd_load_err:
	Print(L"Failed to load initrd: %d\n", status);
	goto end;

end:
	return status;
}
