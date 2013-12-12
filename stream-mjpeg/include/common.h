#ifndef common_H
#define common_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define dbprintf(format, para...) dprintf(1, "%s:%s:%d: "format, __FILE__, __func__, __LINE__,##para)
#else
#define dbprintf(format, para...)
#endif

#include <stdint.h>

#define CONF_DIR "/etc/config"
#define TIMEBUF_LEN (64)
#define ERR_DEV (-513)

struct buffer{
	void *data;
	uint32_t size;
};

#ifdef __cplusplus
}
#endif


#endif
