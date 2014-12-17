#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>

struct blkdev_disk {
	LIST_ENTRY(blkdev_disk) node;
	
	char *name;
	int maj;
	int min;
	unsigned long long size;
};

struct blkdev_part {
	LIST_ENTRY(blkdev_part) node;
	
	char *disk_name;
	char *part_name;
	int maj;
	int min;
};

LIST_HEAD(disk_list, blkdev_disk);
LIST_HEAD(part_list, blkdev_part);

static struct disk_list d_list = LIST_HEAD_INITIALIZER(d_list);
static struct part_list p_list = LIST_HEAD_INITIALIZER(p_list);

static const char * PATH_SYS_BLOCK = "/sys/block";


int add_partition(const char *disk_name, const char *part_name)
{
	struct stat st;
	struct blkdev_part *part;
	char dev_path[64];

	snprintf(dev_path, sizeof(dev_path), "/dev/%s" , part_name);
	if (stat(dev_path, &st))
	{
		perror("[ERROR] stat");
		return -1;
	}
	
	part  = (struct blkdev_part *)calloc(1, sizeof(struct blkdev_part));
	part->disk_name = strdup(disk_name);
	part->part_name = strdup(dev_path);
	part->maj = major(st.st_rdev);
	part->min = minor(st.st_rdev);

	LIST_INSERT_HEAD(&p_list, part, node);

	return 0;
}

int set_partitions(const char *disk_name)
{
	char disk_path[256];
	DIR *dir;
	struct dirent *d;
	
	snprintf(disk_path, sizeof(disk_path), "%s/%s", PATH_SYS_BLOCK, disk_name);

	if (!(dir = opendir(disk_path)))
	{
		perror("[ERROR] opendir");
		return -1;
	}

	while ((d = readdir(dir))!= NULL) {
		if (!strcmp(d->d_name, ".") ||
		    !strcmp(d->d_name, ".."))
			continue;
		
		if (!strncmp(disk_name, d->d_name, strlen(disk_name)))
			add_partition(disk_name, d->d_name);
	}
	
	closedir(dir);
	
	return 0;
}

int set_disk(const char *dir_name)
{
	struct stat st;
	struct blkdev_disk *disk;
	char dev_path[64];
	int fd;
	unsigned long long size;

	snprintf(dev_path, sizeof(dev_path), "/dev/%s" , dir_name);
	if (stat(dev_path, &st))
	{
		perror("[ERROR] stat");
		return -1;
	}
	if ((fd = open(dev_path, O_RDONLY)) < 0)
	{
		if (errno == ENOMEDIUM)
			;
		else 
			perror("[ERROR] open");
		
		return -1;
	}
	
	if (ioctl(fd, BLKGETSIZE64, &size) < 0)
	{
		perror("[ERROR] ioctl:BLKGETSIZE64");
		return -1;
	}
	
	if (size == 0)
		return -1;

	disk = (struct blkdev_disk *)calloc(1, sizeof(struct blkdev_disk));
	disk->name = strdup(dev_path);
	disk->maj = major(st.st_rdev);
	disk->min = minor(st.st_rdev);
	disk->size =size;

	LIST_INSERT_HEAD(&d_list, disk, node);

	set_partitions(dir_name);
	
	return 0;
}


int find_blkdev_disks(void)
{
	DIR *dir;
	struct dirent *d;

	if (!(dir = opendir(PATH_SYS_BLOCK)))
	{
		perror("[ERROR] opendir");
		return -1;
	}

	while ((d = readdir(dir))!= NULL) {
		if (!strcmp(d->d_name, ".") ||
		    !strcmp(d->d_name, ".."))
			continue;

		if (set_disk(d->d_name)) 
			continue;
	}
	
	closedir(dir);
	return 0;
}

void free_part_list(struct part_list *list)
{
	struct blkdev_part *p;

	while (!LIST_EMPTY(list)) {
		p = LIST_FIRST(list);
		free(p->disk_name);
		free(p->part_name);
		LIST_REMOVE(p, node);
		free(p);
	}
}

void free_disk_list(struct disk_list *list)
{
	struct blkdev_disk *d;

	while (!LIST_EMPTY(list)) {
		d = LIST_FIRST(list);
		free(d->name);
		LIST_REMOVE(d, node);
		free(d);
	}
}

void print_part_list(struct part_list *list)
{
	struct blkdev_part *p;

	LIST_FOREACH(p, list, node) {
		printf("disk name : %s\n", p->disk_name);
		printf("part name : %s\n", p->part_name);
		printf("maj  : %d\n", p->maj);
		printf("min  : %d\n", p->min);
		printf("\n");
	}
}

void print_disk_list(struct disk_list *list)
{
	struct blkdev_disk *d;

	LIST_FOREACH(d, list, node) {
		printf("name : %s\n", d->name);
		printf("maj  : %d\n", d->maj);
		printf("min  : %d\n", d->min);
		printf("size : %llu\n", d->size);
		printf("\n");
	}
}

int main()
{

	find_blkdev_disks();
	print_disk_list(&d_list);
	print_part_list(&p_list);
	free_disk_list(&d_list);
	free_part_list(&p_list);
	
	return 0;
}
