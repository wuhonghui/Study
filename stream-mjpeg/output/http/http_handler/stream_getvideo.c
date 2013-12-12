#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "httplib-internal.h"
#include "http-handler.h"
#include "http_digest_auth.h"

#include "input_video.h"
#include "utils/timing.h"

static void free_video_stream_priv(void *priv)
{
	video_remove_receiver((struct generic_receiver *)priv);
}

static int http_video_send_one(void *data, size_t len, void *arg)
{
	struct http_connection *hc = arg;
	int ret = 0;
	int canskip = 0;

	/* remove receiver and connection if data is NULL */
	if (!data) {
        http_connection_free(hc);
		return -1;
	}

	if (evbuffer_get_length(hc->evbout) == 0) {
        /* a full frame can be skip */
        canskip = 1;

        /* boundary and header info */
        evbuffer_add_printf(hc->evbout, "--video-boundary--\r\n");
        evbuffer_add_printf(hc->evbout, "Content-length: %d\r\n", len);
        evbuffer_add_printf(hc->evbout, "Date: %s\r\n", http_server_fmttime());
        evbuffer_add_printf(hc->evbout, "Content-Type: image/jpeg\r\n\r\n");

        /* copy video frame to evbuffer */
        evbuffer_add(hc->evbout, data, len);
        evbuffer_add_printf(hc->evbout, "\r\n");
    }

    /* write out evbuffer */
    ret = http_evbuffer_writeout(hc, canskip);
    if (ret < 0) {
        http_connection_free(hc);
		return -1;
    }

	return 0;
}

void http_stream_getvideo_cb(evutil_socket_t fd, short events, void *arg)
{
    int ret = 0;
    struct http_connection *hc = (struct http_connection *)arg;

	/* do auth */
	if (0 != http_digest_auth_check(hc)) {
		return;
	}

	if (hc->username) {
		char identity[128];

		/* video:protocol:ip:port:username:begin_time */
		snprintf(identity, sizeof(identity), "video:http:%s:%d:%s:%d",
			inet_ntoa(hc->sin.sin_addr),
			ntohs(hc->sin.sin_port),
			hc->username? hc->username : "",
			(int)monotonic_time());
		hc->priv = video_add_receiver(identity, http_video_send_one, hc);
	}else {
		hc->priv = video_add_receiver(NULL, http_video_send_one, hc);
	}

	if (!hc->priv) {
		if (video_device_status() == DEVICE_RUN_STREAMING) {
			http_header_start(hc, 503, "Service Unavailable", "text/html");
		} else {
			http_header_start(hc, 500, "Internal Server Error", "text/html");
		}
		http_header_add(hc, "Pragma: no-cache");
	    http_header_add(hc, "Cache-Control: no-cache");
	    http_header_clength(hc, 0);
	    http_header_end(hc);

    	http_evbuffer_writeout(hc, 0);
		http_connection_free(hc);
		return;
	} else {
		hc->free_priv = free_video_stream_priv;
	}

	/* set http header */
    http_header_start(hc, 200, "OK", "multipart/x-mixed-replace;boundary=video-boundary--");
    http_header_add(hc, "Pragma: no-cache");
    http_header_add(hc, "Cache-Control: no-cache");
    http_header_clength(hc, -1);
    http_header_end(hc);

    ret = http_evbuffer_writeout(hc, 0);
    if (ret < 0) {
        http_connection_free(hc);
        return;
    }
}

