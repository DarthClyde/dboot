#include "protos/boot.h"

#include "protos/linux.h"

error_t boot_boot(config_entry_t* config)
{
	error_t error = ERR_OK;

	switch (config->type)
	{
		case ENTRY_TYPE_LINUX:
		{
			error = linux_boot(config->kernel_path, config->module_path, config->cmdline);
			break;
		}

		case ENTRY_TYPE_EFI:
		case ENTRY_TYPE_GROUP:
		default:
		{
			error = ERR_BOOT_UNSUPPORTED;
			break;
		}
	}

	return error;
}
