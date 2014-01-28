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

#include "media_source.h"
#include "media_receiver.h"

static void free_media_receiver_priv(void *priv)
{
	struct media_receiver *receiver = (struct media_receiver *)priv;
	media_source_del_receiver(receiver);
	media_receiver_free(receiver);
}

static int http_video_h264_send_one(struct frame_buf *buf, void *arg)
{
	struct http_connection *hc = arg;
	int ret = 0;
	int canskip = 0;

	/* remove receiver and connection if data is NULL */
	if (!buf) {
        http_connection_free(hc);
		return -1;
	}

	if (evbuffer_get_length(hc->evbout) == 0) {
        /* a full frame can be skip */
        canskip = 1;

        /* boundary and header info */
        evbuffer_add_printf(hc->evbout, "--video-boundary--\r\n");
        evbuffer_add_printf(hc->evbout, "Content-Length: %d\r\n", buf->size);
        evbuffer_add_printf(hc->evbout, "Date: %s\r\n", http_server_fmttime());
		evbuffer_add_printf(hc->evbout, "X-Timestamp: %ld.%ld\r\n", buf->timestamp.tv_sec, buf->timestamp.tv_usec);
        evbuffer_add_printf(hc->evbout, "Content-Type: video/x-h264\r\n\r\n");

        /* copy video frame to evbuffer */
        evbuffer_add(hc->evbout, buf->data, buf->size);
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

void http_stream_video_h264_cb(evutil_socket_t fd, short events, void *arg)
{
    int ret = 0;
    struct http_connection *hc = (struct http_connection *)arg;
	struct media_source *source = NULL;
	struct media_receiver *receiver = NULL;

	// TODO: find media source by request parameter
	source = server_media_source_find("LIVE-H264-QVGA");
	if (!source) {
		http_connection_free(hc);
		return;
	}
	
	receiver = media_receiver_new();
	if (!receiver) {
		http_connection_free(hc);
		return;
	}
	media_receiver_set_callback(receiver, http_video_h264_send_one, hc);
	
	hc->priv = receiver;
	hc->free_priv = free_media_receiver_priv;

	media_source_add_receiver(source, receiver);

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
