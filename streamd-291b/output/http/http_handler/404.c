#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

#include "common.h"
#include "httplib-internal.h"
#include "http-handler.h"

static const char *str_http_not_found =
"<!DOCTYPE HTML>\
<html>\
<head>\
<title>404 Not Found</title>\
</head>\
<body>\
<h1>404 Not Found</h1>\
</body>\
</html>";

void http_404_cb(evutil_socket_t fd, short events, void *arg)
{
	int ret = 0;
	struct http_connection *hc = (struct http_connection *)arg;

	http_header_start(hc, 404, "Not Found", "text/html");
	http_header_clength(hc, strlen(str_http_not_found));
	http_header_end(hc);

	evbuffer_add(hc->evbout, str_http_not_found, strlen(str_http_not_found));

	http_evbuffer_writeout(hc, 0);

	http_connection_free(hc);
}

