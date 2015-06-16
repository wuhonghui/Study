#ifndef slog_H
#define slog_H

#ifdef __cplusplus
extern "C" {
#endif

enum SLOG_LEVEL {
	SLOG_DEBUG = 0,
	SLOG_INFO,
	SLOG_NOTICE,
	SLOG_WARN,
	SLOG_ERROR,
	SLOG_FATAL,
	SLOG_END
};

void __slog(int32_t level, char *format, ...) \
	__attribute__((format(printf, 2, 3)));
void slog_setlevel(int32_t level);

#define slog(level, format, args...) \
	do { \
		__slog(level, "%s:%d - "format, __FILE__, __LINE__, ##args); \
	} while(0)

#ifdef __cplusplus
}
#endif

#endif
