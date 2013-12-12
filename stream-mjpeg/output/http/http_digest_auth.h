#ifndef http_digest_auth_H
#define http_digest_auth_H

#ifdef __cplusplus
extern "C" {
#endif

#include "httplib-internal.h"

int http_digest_auth_check(struct http_connection *hc);

#ifdef __cplusplus
}
#endif

#endif
