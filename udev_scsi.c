#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <libudev.h>

/* gcc -o udev_scsi udev_scsi.c -ludev
 * 
*/
 
/******************************************************************************
**  Name        :   find_scsi_host_num
**  Description :   find host number
**  Input       :   char *dev_name (sdx)
**  Output      :   host number
******************************************************************************/
int find_scsi_host_num(char *dev_name)
{
	char dev_path[1024];
	DIR *dir;
	char *base;
	char *name;
	int host = -1, bus, target, lun;

	struct udev *context = udev_new();
	struct udev_enumerate *enumerator = udev_enumerate_new(context);

	udev_enumerate_add_match_subsystem(enumerator, "scsi");
	udev_enumerate_scan_devices(enumerator);

	struct udev_list_entry *scsi_devices = udev_enumerate_get_list_entry(enumerator);
	struct udev_list_entry *current = 0;

	udev_list_entry_foreach(current, scsi_devices) {
    		struct udev_device *device = udev_device_new_from_syspath(
            					context, udev_list_entry_get_name(current));
  
		base = strdup(udev_device_get_syspath(device));

		snprintf(dev_path, sizeof(dev_path),"%s/block/%s", base, dev_name);
		free(base);

		dir = opendir(dev_path);
		if (dir == NULL) 
		{
			continue;
		} 
		else 
		{
			char *name = strdup(udev_device_get_sysname(device));
			printf("sys name: %s\n", name);
			
			if (sscanf(name, "%d:%d:%d:%d", &host, &bus, &target, &lun) != 4)
				host = -1;
			
			free(name);
			break;
		}
	}

	return host;
}

int main (int argc, char *argv[])
{
	int host;
	
	if (argc < 2) {
		printf("Incorrect Input\n");
		return;
	}

	printf("Find the SCSI host of %s\n", argv[1]);
	
	
	if ((host = find_scsi_host_num(argv[1])) != -1)
		printf("host: %d\n", host);
}



