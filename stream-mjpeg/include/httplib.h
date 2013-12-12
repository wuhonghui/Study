#ifndef httplib_H
#define httplib_H

#ifdef __cplusplus
extern "C" {
#endif
#include "event2/event.h"

struct http_server;

/* Server */
struct http_server * http_server_new(struct event_base *base, int port);

void http_server_free(struct http_server *hs);

#ifdef __cplusplus
}
#endif

#endif
