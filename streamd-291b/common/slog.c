#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include "slog.h"

#define LOG_BUFFER_SIZE 4096*16

static int log_level = SLOG_DEBUG;
static const char *month[] = {	
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static char *log_data = NULL;
static int log_in, log_out, log_size;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int slog_init(int level)
{
	log_level = level;
	log_in = 0;
	log_out = 0;
	log_size = LOG_BUFFER_SIZE;
	log_data = malloc(log_size);
	return 0;
}

void slog_level(int level)
{
	pthread_mutex_lock(&log_mutex);
	log_level = level;
	pthread_mutex_unlock(&log_mutex);
}

void slog_send_buffer(int fd)
{
	pthread_mutex_lock(&log_mutex);
	if (log_in < log_out) {
		write(fd, log_data + log_out, log_size - log_out);
		write(fd, log_data, log_in);
	} else {
		write(fd, log_data + log_out, log_in - log_out);
	}
	pthread_mutex_unlock(&log_mutex);
}

void slog(int level, char *format, ...)
{
	va_list ap;
	char newfmt[512], line[1024];
	struct tm tm;
	time_t t;
	int i, len, overwrite = 0;

	va_start(ap, format);
	time(&t);
	localtime_r(&t, &tm);
	snprintf(newfmt, 512, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
		tm.tm_year+1900, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		format);
	len = vsnprintf(line, sizeof(line), newfmt, ap);
	va_end(ap);

	if (level >= log_level) {
		fputs(line, stderr);
	}

	pthread_mutex_lock(&log_mutex);
	for (i = 0; i < len; ++i) {
		log_data[log_in++] = line[i];
		if (log_in == log_size) {
			log_in = 0;
		}
		if (log_in == log_out) {
			overwrite = 1;
		}
	}
	if (overwrite) {
		log_out = log_in;
		while (log_data[log_out] != '\n') {
			if (++log_out == log_size)
				log_out = 0;
		}
		if (++log_out == log_size)
			log_out = 0;
	}
	pthread_mutex_unlock(&log_mutex);
}


