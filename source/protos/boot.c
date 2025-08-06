#include "protos/boot.h"

#include <efilib.h>

#include "protos/linux.h"

error_t boot_boot(config_entry_t* config)
{
	error_t error = ERR_OK;

	switch (config->type)
	{
		case ENTRY_TYPE_LINUX:
		{
			path_t* kernel_path = NULL;
			path_t* initrd_path = NULL;

			// Parse kernel path
			error = path_parse(config->kernel_path, &kernel_path);
			if (error)
			{
				ERR_PRINT(error);
				error = ERR_BOOT_FAIL_LINUX;
				break;
			}

			// Parse initrd path
			error = path_parse(config->module_path, &initrd_path);
			if (error)
			{
				ERR_PRINT(error);
				error = ERR_BOOT_FAIL_LINUX;
				break;
			}

			// Boot Linux
			error = linux_boot(kernel_path, initrd_path, config->cmdline);
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
