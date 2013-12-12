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
#include "http_digest_auth.h"


static const char *str_http_index_html =
"<!DOCTYPE HTML>\
<html>\
<head>\
<title>TL-SC2000N</title>\
</head>\
<body>\
<img src=\"/stream/getvideo\">\
</body>\
</html>";

/*

add this to index html to enable audio

<audio src=\"/stream/getaudio\" controls=\"controls\" preload=\"metadata\" autoplay=\"autoplay\">wav</audio>\
<object id=\"player\" height=\"64\" width=\"264\" classid=\"CLSID:6BF52A52-394A-11d3-B153-00C04F79FAA6\">\
    <param NAME=\"AutoStart\" VALUE=\"1\">\
    <param NAME=\"url\" value=\"/stream/getaudio\">\
</object>\

*/

void http_gen_cb(evutil_socket_t fd, short events, void *arg)
{
	int ret = 0;
	struct http_connection *hc = (struct http_connection *)arg;

	if (0 != http_digest_auth_check(hc)) {
		return;
	}

	http_header_start(hc, 200, "OK", "text/html");
	http_header_clength(hc, strlen(str_http_index_html));
	http_header_end(hc);

	evbuffer_add(hc->evbout, str_http_index_html, strlen(str_http_index_html));

	http_evbuffer_writeout(hc, 0);

	http_connection_free(hc);
}
