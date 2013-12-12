#ifndef timing_H
#define timing_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

void simple_timing_begin(struct timeval *begin);
int simple_timing_end(struct timeval *begin);

void timeval_to_str(struct timeval *t, char *buf, int bufsize);
float timeval_to_float(struct timeval *a);

void timeval_add(struct timeval *a, struct timeval *b, struct timeval *c);
void timeval_sub(struct timeval *a, struct timeval *b, struct timeval *c);

struct timing_helper;
struct timing_helper * timing_helper_new(char *name);
void timing_helper_reset(struct timing_helper *t);
void timing_helper_begin(struct timing_helper *t);
void timing_helper_end(struct timing_helper *t);
void timing_helper_show(struct timing_helper *t);
void timing_helper_free(struct timing_helper *t);

#ifdef __cplusplus
}
#endif


#endif
