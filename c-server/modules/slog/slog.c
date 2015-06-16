#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include "slog/slog.h"

static int32_t slog_print = 1;
static char   *slog_file  = NULL;
static int32_t slog_level = SLOG_DEBUG;
static char   *slog_level_str[] = {
	"DEBUG",
	"INFO",
	"NOTICE",
	"WARN",
	"ERROR",
	"FATAL"
};

void __slog(int32_t level, char *format, ...)
{
	struct tm tm;
	struct timeval tv;
	va_list ap;
	int32_t log_length;
	char log_buffer[1024];

	if (level < SLOG_DEBUG || level > SLOG_END)
		return;
	if (level < slog_level)
		return;

	gettimeofday(&tv, NULL);
	localtime_r(&(tv.tv_sec), &tm);
	log_length = snprintf(log_buffer, sizeof(log_buffer), 
		"[%04d-%02d-%02d %02d:%02d:%02d.%06d] %s ", tm.tm_year + 1900, tm.tm_mon, 
		tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (int32_t)tv.tv_usec, slog_level_str[level]);

	va_start(ap, format);
	log_length += vsnprintf(log_buffer + log_length, 
		sizeof(log_buffer) - log_length, format, ap);
	log_buffer[sizeof(log_buffer) - 2] = '\n';
	va_end(ap);

	if (slog_print)	{
		fputs(log_buffer, stderr);
	}

	if (slog_file) {
		// TODO: save to file
	}
}

void slog_setlevel(int32_t level)
{
	slog_level = level;
}
