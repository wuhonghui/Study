#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include "common.h"
#include "httplib-internal.h"
#include "http-handler.h"
#include "http_digest_auth.h"

#include "input_video.h"

static void free_snapshot_priv(void *priv)
{
	video_remove_receiver((struct generic_receiver *)priv);
}

static int http_send_snapshot(void *data, size_t len, void *arg)
{
	struct http_connection *hc = arg;
	int ret = 0;

	if (evbuffer_get_length(hc->evbout) == 0) {
	    http_header_start(hc, 200, "OK", "image/jpg");
	    http_header_add(hc, "Pragma: no-cache");
	    http_header_add(hc, "Cache-Control: no-cache");
	    http_header_clength(hc, len);
	    http_header_end(hc);

		evbuffer_add(hc->evbout, data, len);
	}

    /* write out evbuffer */
    ret = http_evbuffer_writeout(hc, 0);
    if (ret < 0) {
        http_connection_free(hc);
		return -1;
    }

	if (evbuffer_get_length(hc->evbout) == 0) {
		http_connection_free(hc);
	}
	return 0;
}

void http_get_snapshot_cb(evutil_socket_t fd, short events, void *arg)
{
    int ret = 0;
    struct http_connection *hc = (struct http_connection *)arg;

	if (0 != http_digest_auth_check(hc)) {
		return;
	}

	hc->priv = video_add_receiver(NULL, http_send_snapshot, hc);
	if (!hc->priv) {
		http_connection_free(hc);
		return;
	}
	hc->free_priv = free_snapshot_priv;
}

