#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slog/slog.h"

int main(int argc, char *argv[])
{
	slog(SLOG_DEBUG, "hello world\n");
	return 0;
}
