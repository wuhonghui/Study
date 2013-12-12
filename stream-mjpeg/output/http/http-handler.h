#ifndef http_handler_H
#define http_handler_H

#ifdef __cplusplus
extern "C" {
#endif


void http_gen_cb(evutil_socket_t fd, short events, void *arg);

void http_404_cb(evutil_socket_t fd, short events, void *arg);

void http_500_cb(evutil_socket_t fd, short events, void *arg);

void http_stream_getvideo_cb(evutil_socket_t fd, short events, void *arg);

void http_stream_getaudio_cb(evutil_socket_t fd, short events, void *arg);

void http_get_snapshot_cb(evutil_socket_t fd, short events, void *arg);

void http_get_video_resolution_cb(evutil_socket_t fd, short events, void *arg);

#ifdef __cplusplus
}
#endif

#endif

