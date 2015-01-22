#define  _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libudev.h>
 
struct blockdev_stats {
    unsigned long long  read_ops;       /* Number of read I/Os processed */
    unsigned long long  read_merged;    /* Number of read I/Os merged with in-queue I/O */
    unsigned long long  read_512bytes;  /* Number of sectors read */
    unsigned long long  read_waits_ms;  /* Total wait time for read requests, milliseconds */
    unsigned long long  write_ops;      /* Number of write I/Os processed */
    unsigned long long  write_merged;   /* Number of write I/Os merged with in-queue I/O */
    unsigned long long  write_512bytes; /* Number of sectors written */
    unsigned long long  write_waits_ms; /* Total wait time for write requests, milliseconds */
    unsigned long long  in_flight;      /* Number of I/Os currently in flight */
    unsigned long long  active_ms;      /* Total active time, milliseconds */
    unsigned long long  waits_ms;       /* Total wait time, milliseconds */
};
 
/* Returns nonzero if stats is filled with the above values.
*/
static inline int get_blockdev_stats(struct udev_device *const device, struct blockdev_stats *const stats)
{
    if (device != NULL && stats != NULL) {
        const char *const s = udev_device_get_sysattr_value(device, "stat");
        if (s == NULL || *s == '\0')
            return 0;
        if (sscanf(s, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &(stats->read_ops), &(stats->read_merged), &(stats->read_512bytes), &(stats->read_waits_ms),
                   &(stats->write_ops), &(stats->write_merged), &(stats->write_512bytes), &(stats->write_waits_ms),
                   &(stats->in_flight), &(stats->active_ms), &(stats->waits_ms)) >= 11)
            return 11;
    }
    return 0;
}
 
/* NULL pointer safe string comparison.
 * Returns 1 if neither pointer is NULL, and the strings they point to match.
*/
static inline int eq(const char *const a, const char *const b)
{
    if (a == NULL || b == NULL)
        return 0;
    else
        return !strcmp(a, b);
}
 
/* NULL pointer safe string to long long conversion.
*/
static inline long long stoll(const char *const string, const long long error)
{
    if (!string || *string == '\0')
        return error;
    return strtoll(string, NULL, 10);
}
 
int main(void)
{
    struct udev            *udevhandle;
    struct udev_enumerate  *blockdevices;
    struct udev_list_entry *entry;
    const char             *devicepath;
    struct udev_device     *device;
 
    /* Get an udev handle. */
    udevhandle = udev_new();
    if (!udevhandle) {
        fprintf(stderr, "Cannot get udev handle.\n");
        return 1;
    }
 
    /* Enumerate block devices. */
    blockdevices = udev_enumerate_new(udevhandle);
    udev_enumerate_add_match_subsystem(blockdevices, "block");
    udev_enumerate_scan_devices(blockdevices);
 
    /* Iterate through the enumerated list. */
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(blockdevices)) {
        /* Use the devpath in the list to obtain a reference to the device. */
        devicepath = udev_list_entry_get_name(entry);

        device = udev_device_new_from_syspath(udevhandle, devicepath);
 
        /* Do the following only for "disk" devtypes in "block" subsystem: */
        if (eq("disk", udev_device_get_devtype(device))) {
 
            const char *const name = udev_device_get_sysname(device);

            const char *const dev = udev_device_get_devnode(device); /* In /dev */
            const char *const bus = udev_device_get_property_value(device, "ID_BUS");
            const char *const model = udev_device_get_property_value(device, "ID_MODEL");
            const char *const serial = udev_device_get_property_value(device, "ID_SERIAL");
            const char *const serial_short = udev_device_get_property_value(device, "ID_SERIAL_SHORT");
            const char *const revision = udev_device_get_property_value(device, "ID_REVISION");
            const char *const vendor = udev_device_get_sysattr_value(device, "device/vendor");
            const char *const rev = udev_device_get_sysattr_value(device, "device/rev");
            const long        size = stoll(udev_device_get_sysattr_value(device, "size"), -2048LL) / 2048LL;
            const long        removable = stoll(udev_device_get_sysattr_value(device, "removable"), -1LL);
            const long        readonly = stoll(udev_device_get_sysattr_value(device, "ro"), -1LL);
            struct blockdev_stats stats;
 
            if (dev)
                printf("%s:\n", dev);
            else
                printf("Unknown device:\n");
 
            if (size > 0L)
                printf("\tSize: %ld MiB\n", size);
 
            if (removable == 0L)
                printf("\tRemovable: No\n");
            else
            if (removable == 1L)
                printf("\tRemovable: Yes\n");
 
            if (readonly == 0L)
                printf("\tRead-only: No\n");
            else
            if (readonly == 1L)
                printf("\tRead-only: Yes\n");
 
            if (name)
                printf("\tName: '%s'\n", name);

            if (bus)
                printf("\tBus: '%s'\n", bus);
 
            if (vendor)
                printf("\tVendor: '%s'\n", vendor);
 
            if (model)
                printf("\tModel: '%s'\n", model);
 
            if (serial)
                printf("\tSerial: '%s'\n", serial);
 
            if (serial_short)
                printf("\tShort serial: '%s'\n", serial_short);
 
            if (revision)
                printf("\tRevision: '%s' (udev)\n", revision);
 
            if (rev)
                printf("\tRevision: '%s' (sys)\n", rev);
 
            if (get_blockdev_stats(device, &stats)) {
                printf("\tRead: %llu requests, %llu MiB\n", stats.read_ops, stats.read_512bytes / 2048ULL);
                printf("\tWritten: %llu requests, %llu MiB\n", stats.write_ops, stats.write_512bytes / 2048ULL);
            }
        }
 
        /* Drop the device reference. */
        udev_device_unref(device);
    }
 
    /* Release the enumerator object, and the udev handle. */
    udev_enumerate_unref(blockdevices);
    udev_unref(udevhandle);
 
    return 0;
}
