#ifndef SLOG_H_
#define SLOG_H_

enum {
	SLOG_DEBUG=0,
	SLOG_INFO,
	SLOG_NOTICE,
	SLOG_WARN,
	SLOG_ERROR,
	SLOG_FATAL
};

void slog(int level, char *format, ...);
int  slog_init(int level);
void slog_level(int level);
void slog_send_buffer(int fd);

#ifdef DEBUG
/* log to console will have many time lost, remove it when released.*/
#define SLOG(level, format, args...) slog(level, "%s:%d - "format, __FILE__, __LINE__, ##args);
#define SLOG_INIT(level) slog_init(level);
#define SLOG_LEVEL(level) slog_level(level);
#define SLOG_SEND_BUFFER(fd) slog_send_buffer(fd);
#else
#define SLOG(level, format, args...)
#define SLOG_INIT(level)
#define SLOG_LEVEL(level)
#define SLOG_SEND_BUFFER(fd)
#endif

#endif
