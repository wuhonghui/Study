#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "common.h"
#include "utils/envirment.h"

#ifdef DEBUG
#define _CORE_SIZE (10 * 1024 * 1024)
#else 
#define _CORE_SIZE (0)
#endif

static void redirent_std_out()
{
	int fd = open("/dev/console", O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd != -1) {
		dup2(fd, 1);
		close(fd);
	}
}
static void reditent_std_err()
{
	int fd = open("/dev/console", O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd != -1) {
		dup2(fd, 2);
		close(fd);
	}
}

void envirment_signal_handler()
{
	//TODO:
}

void envirment_std_redirent()
{
	redirent_std_out();
	reditent_std_err();
}

void set_core_limit()
{
	//TODO:
}

void set_oom_adj()
{
	//TODO:
}
