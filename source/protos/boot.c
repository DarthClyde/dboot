#include "protos/boot.h"

#include "protos/linux.h"

EFI_STATUS boot_boot(config_entry_t* config)
{
	EFI_STATUS status = EFIERR(99);

	switch (config->type)
	{
		case ENTRY_TYPE_LINUX:
		{
			status = linux_boot(config->kernel_path, config->module_path, config->cmdline);
			break;
		}

		case ENTRY_TYPE_EFI:
		case ENTRY_TYPE_GROUP:
		default:
		{
			status = EFI_UNSUPPORTED;
			break;
		}
	}

	return status;
}
