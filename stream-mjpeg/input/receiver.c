#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include "common.h"
#include "receiver.h"

int receiver_identity_init()
{
	system("mkdir -p "RECEIVER_DIR);
	system("rm -rf "RECEIVER_DIR"*");

	return 0;
}

int receiver_identity_clear()
{
	system("rm -rf "RECEIVER_DIR"*");
	return 0;
}

int receiver_identity_del(const char *identity)
{
	char fullpath[128];
	struct stat file_stat;
	int fd = 0;

	if (!identity)
		return -1;

	snprintf(fullpath, sizeof(fullpath), "%s%s", RECEIVER_DIR, identity);

	if (0 != lstat(fullpath, &file_stat)) {
		return -1;
	}

	if (unlink(fullpath)) {
		printf("unlink file error\n");
		return -1;
	}
	return 0;
}

int receiver_identity_add(const char *identity)
{
	char fullpath[128];
	struct stat file_stat;
	int fd = 0;

	if (!identity)
		return -1;

	snprintf(fullpath, sizeof(fullpath), "%s%s", RECEIVER_DIR, identity);

	if (0 == lstat(fullpath, &file_stat)) {
		printf("identity exist!!!!!!!!!!!!!!\n");
		return -1;
	}

	if (errno != ENOENT) {
		printf("stat file error, errno: %d\n", errno);
		return -1;
	}

	fd = open(fullpath, O_CREAT|O_EXCL|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO);
	if (fd < 0) {
		printf("creat file failed.\n");
		return -1;
	}
	close(fd);

	return 0;
}

int receiver_identity_is_exist(const char *identity)
{
	char fullpath[128];
	struct stat file_stat;
	int fd = 0;

	if (!identity)
		return 0;

	snprintf(fullpath, sizeof(fullpath), "%s%s", RECEIVER_DIR, identity);

	if (0 == lstat(fullpath, &file_stat)) {
		if (S_ISREG(file_stat.st_mode)) {
			return 1;
		}
	}

	return 0;
}

