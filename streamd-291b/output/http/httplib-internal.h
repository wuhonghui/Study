#ifndef httplib_internal_H
#define httplib_internal_H

#ifdef __cplusplus
extern "C" {
#endif
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/listener.h"

/* ADD MORE STATUS IF NEEDED */
enum HTTP_CONNECTION_STATUS{
	HTTP_CONNECTION_STATUS_READ_HEADER,
	HTTP_CONNECTION_STATUS_READ_BODY,
	HTTP_CONNECTION_STATUS_SEND_MJPEG_STREAM,
	HTTP_CONNECTION_STATUS_END,
	HTTP_CONNECTION_STATUS_RTSP_SESSION,
};

/* Response codes */
#define HTTP_ERROR		-1
#define HTTP_OK			200	/**< request completed ok */
#define HTTP_NOCONTENT		204	/**< request does not have content */
#define HTTP_MOVEPERM		301	/**< the uri moved permanently */
#define HTTP_MOVETEMP		302	/**< the uri moved temporarily */
#define HTTP_NOTMODIFIED	304	/**< page was not modified from last */
#define HTTP_BADREQUEST		400	/**< invalid http request was made */
#define HTTP_NOTFOUND		404	/**< could not find content for uri */
#define HTTP_BADMETHOD		405 	/**< method not allowed for this uri */
#define HTTP_ENTITYTOOLARGE	413	/**<  */
#define HTTP_EXPECTATIONFAILED	417	/**< we can't handle this expectation */
#define HTTP_INTERNAL           500     /**< internal error */
#define HTTP_NOTIMPLEMENTED     501     /**< not implemented */
#define HTTP_SERVUNAVAIL	503	/**< the server is not available */

/* Structure definition */
struct http_server;
struct http_connection;
struct http_uri;

struct name_value
{
	char	*name;
	char    *value;
	struct name_value *next;
};

struct http_uri{
	char 			*uri;
	void 			(*handler_cb)(evutil_socket_t fd, short events, void *arg);
	struct http_uri *next;
};

struct http_connection{
	/* http server */
	struct http_server 	*hs;

	int 				fd;
	struct sockaddr_in 	sin;
	int 				connection_status;

	/* event and event buffer */
	struct event 		*ev_read;
	struct event		*ev_write;

	struct evbuffer		*evbin;
	struct evbuffer		*evbout;

	/* http elements */
	char *username;
	char *method;
	int	 status;
	char *uri;
	char proto_major;
	char proto_minor;

	struct name_value *request_header;
	struct name_value *query_params;

	/* priv data*/
	void *priv;
	void (*free_priv)(void *);

	/* statistics */
	int consecutive_failed;

	/* dlist */
	struct http_connection *prev;
	struct http_connection *next;
};


struct http_server {
	struct event_base 		*base;		/* event base*/
	struct evconnlistener 	*listener;	/* listener */
	struct http_connection 	*hc;		/* connections */
	struct http_uri 		*uris;		/* uris and handlers */
};

/* time system startup */
time_t monotonic_time(void);
long monotonic_time_nsec(void);

/* return server formated time string */
char * http_server_fmttime();

/* Connection */
void http_connection_free(struct http_connection *hc);

/* Query */
char * http_get_query_param(struct http_connection *hc, const char *name);

char * http_get_request_header(struct http_connection *hc, const char *name);

/* Output Header */
int http_header_start(struct http_connection *hc,
	int status, char *result, char *type) ;

int http_header_add(struct http_connection *hc, char *header);

int http_header_clength(struct http_connection *hc, ssize_t length);

int http_header_end(struct http_connection *hc);

int http_evbuffer_writeout(struct http_connection *hc, int can_skip);

#ifdef __cplusplus
}
#endif

#endif
