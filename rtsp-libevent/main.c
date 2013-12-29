#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/listener.h>

#include "rtsp/rtsp.h"
#include "slog.h"

static void signal_quit_cb(evutil_socket_t sig, short events, void *arg)
{
	struct event_base *base = arg;
	struct timeval delay = { 0, 5 * 1000 };

    SLOG(SLOG_NOTICE, "server received signal %d", sig);

	event_base_loopexit(base, &delay);
}

int main(int argc, char *argv[])
{
	struct event_base *base = NULL;

	struct event *sigint_event = NULL;
	struct event *sigterm_event = NULL;
	
	struct rtsp_server_param rs_param;
	struct rtsp_server *rs = NULL;

    slog_init(SLOG_DEBUG);

	/* create event base */
	base = event_base_new();
	if (!base) {
		SLOG(SLOG_ERROR, "failed to create event base");
		return -1;
	}
	
	/* install signal handle */
	sigint_event = evsignal_new(base, SIGINT, signal_quit_cb, (void *)base);
	if (!sigint_event) {
		SLOG(SLOG_ERROR, "failed to create sigint event");
		goto failed;
	}
	sigterm_event = evsignal_new(base, SIGTERM, signal_quit_cb, (void *)base);
	if (!sigterm_event) {
		SLOG(SLOG_ERROR, "failed to create sigterm event");
		goto failed;
	}
	event_add(sigint_event, NULL);
	event_add(sigterm_event, NULL);

	/* init rtsp-server param */
	rs_param.port = 8554;
	rs_param.enable_http_tunnel = 1;
	rs_param.http_tunnel_port = 8080;
	rs_param.udp_port_range[0] = 20000;
	rs_param.udp_port_range[1] = 30000;
	rs_param.max_sessions = 10;

	/* start up rtsp-server */
	rs = rtsp_server_new(base, &rs_param);
	if (!rs) {
		SLOG(SLOG_ERROR, "failed to create rtsp server");
		goto failed;
	}
	
	/* start eventloop */
	event_base_dispatch(base);
	/* fall throught when normal exit */

    SLOG(SLOG_NOTICE, "server exit");
failed:
	if (rs)
		rtsp_server_free(rs);
	
	/* uninstall signal handler */
	if (sigterm_event)
		evsignal_del(sigterm_event);
	if (sigint_event)
		evsignal_del(sigint_event);
	
	if (base)
		event_base_free(base);

	return 0;
}
