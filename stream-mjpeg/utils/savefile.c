#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "utils/savefile.h"

void savejpg(unsigned char *addr, unsigned int size)
{
	int filefd;
	static int m = 0;
	char filename[32];
	snprintf(filename, 32, "%s-%d.jpg", "capature", m++);
	printf("save to jpg file: %s, size: %d\n", filename, size);
	filefd = open(filename, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	write(filefd, addr, size);
	close(filefd);
}
void savefile(char *filename, unsigned char *addr, unsigned int size)
{
	int filefd;
	filefd = open(filename, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	write(filefd, addr, size);
	close(filefd);
}
