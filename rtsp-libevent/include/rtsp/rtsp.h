#ifndef RTSP_H
#define RTSP_H

struct rtsp_server_param {
	int port;
	int enable_http_tunnel;
	int http_tunnel_port;
	int udp_port_range[2];

	int max_sessions;
};

struct rtsp_server;


struct rtsp_server * rtsp_server_new(struct event_base *base, 
		struct rtsp_server_param *param);

void rtsp_server_free(struct rtsp_server *server);

#endif
