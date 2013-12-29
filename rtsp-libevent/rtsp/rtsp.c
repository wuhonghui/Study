#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/buffer_compat.h>
#include <event2/listener.h>

#include "rtsp/rtsp.h"
#include "rtsp/session.h"

#include "slog.h"

#define MAX_REQUEST_HEADER_LENGTH 4096

struct rtsp_server {
	struct rtsp_server_param 	*param;
	
	int n_sessions;
	struct rtsp_session     *session_head;

	int n_connections;
	struct connection       *conn_head;

	struct event_base 		*base;
	struct evconnlistener 	*listener;
	struct evconnlistener 	*http_listener;
};

enum CONNECTION_PROTO{
    CONNECTION_PROTO_RAW_TCP,
    CONNECTION_PROTO_RTSP,
    CONNECTION_PROTO_HTTP,
    CONNECTION_PROTO_SIP,
};

struct key_value {
    char *key;
    char *value;
    struct key_value *next;
};

struct key_value *
key_value_new(char *key, char *value)
{
    struct key_value *kv = NULL;
    kv = calloc(1, sizeof(struct key_value));
    if (!kv) {
        SLOG(SLOG_DEBUG, "failed to alloc new key_value");
        return NULL;
    }
    if (key)
        kv->key = strdup(key);
    if (value)
        kv->value = strdup(value);
    kv->next = NULL;

    return kv;
}

void
key_value_free(struct key_value *kv)
{
    if (kv) {
        SLOG(SLOG_DEBUG, "key_value_free: '%s' : '%s'", kv->key, kv->value);
        if (kv->key)
            free(kv->key);
        if (kv->value)
            free(kv->value);
        free(kv);
    }
}

/* free a key_value list */
void
key_value_list_free(struct key_value *kv_head)
{
    struct key_value *next = NULL;

    while (kv_head) {
        next = kv_head->next;
        key_value_free(kv_head);
        kv_head = next;
    }
}

/* add a new kv to list kv_list */
void
key_value_list_add(struct key_value **kv_list, struct key_value *kv)
{
    if (!kv_list || !kv)
        return;

    if (*kv_list == NULL) {
        *kv_list = kv;
        return;
    }

    kv->next = *kv_list;
    *kv_list = kv;
}

char *
key_value_list_find(const struct key_value* kv_list, const char *key)
{
    while (kv_list) {
        if (strcmp(kv_list->key, key) == 0) {
            return kv_list->value;
        }
        kv_list = kv_list->next;
    }
    return NULL;
}

struct request {
    char *method;               /* need free when request end(entire firstline) */
    char *host;                 /* may null when request uri is relative */
    char *port;                 /* may null when request uri is relative */
    char *uri;
    char *protocol;             /* may null when request uri is relative */
    char *protocol_version;

    struct key_value *header;
    struct key_value *param;
};

struct request * request_new()
{
    struct request *req = NULL;
    req = (struct request *)calloc(1, sizeof(struct request));
    if (!req) {
        return NULL;
    }
    return req;
}

void request_free(struct request *req)
{
    if (req) {
        if (req->method)
            free(req->method);
        if (req->header) {
            key_value_list_free(req->header);
        }
        if (req->param) {
            key_value_list_free(req->param);
        }
        free(req);
    }
}

static int 
request_string_unescape(char * string)
{
    int length = 0;
    int i = 0;
    int pos = 0;
    char          buffer[3];
    char          tmp;

    if (!string)
    	return 0;

    length = strlen(string);
    i = 0;
    pos = 0;

    while (i < length) {
	    if (string[i] == '+') {
	        string[pos] = ' ';                    	/* replace '+' by spaces */
	    } else if ((string[i] == '%') && (i < length-2)) {
	        if (isxdigit (string[i+1]) && isxdigit (string[i+2])) {
	            buffer[0] = string[i+1];
	            buffer[1] = string[i+2];
	            buffer[2] = '\0';
	            tmp = (char) strtol (buffer, NULL, 16);
	            i += 2;
	            string[pos] = tmp;       			/* replace % by new char */
            }
	    } else {
	        string[pos] = string[i];
	    }

	    pos++;
	    i++;
    }

    string[pos] = '\0';
    
    return (0);
}

static int
request_parse_param(struct request *req, char *param)
{
    char *param_ptr = NULL;
    char *param_value = NULL;

    while ((param_value = strtok_r(param, "&", &param_ptr)) != NULL) {
        char *key = NULL;
        char *value = NULL;
        char *token = NULL;
        struct key_value *kv = NULL;

        key = param_value;
        token = strchr(key, '=');
        if (token != NULL) {
            *token = '\0';
            value = token + 1;
        }

        request_string_unescape(key);
        request_string_unescape(value);

        kv = key_value_new(key, value);
        key_value_list_add(&(req->param), kv);
        
        param = NULL;
    }

    return 0;
}

void request_dump(struct request *req)
{
    if (req) {
        SLOG(SLOG_DEBUG, "method: '%s'", req->method);
        SLOG(SLOG_DEBUG, "host: '%s'", req->host);
        SLOG(SLOG_DEBUG, "port: '%s'", req->port);
        SLOG(SLOG_DEBUG, "uri: '%s'", req->uri);
        SLOG(SLOG_DEBUG, "protocol: '%s'", req->protocol);
        SLOG(SLOG_DEBUG, "protocol_version: '%s'", req->protocol_version);

        struct key_value *kv =  req->param;
        SLOG(SLOG_DEBUG, "request params: ");
        while (kv) {
            SLOG(SLOG_DEBUG, "\t'%s' : '%s'", kv->key, kv->value);
            kv = kv->next;
        }

        kv =  req->header;
        SLOG(SLOG_DEBUG, "request headers: ");
        while (kv) {
            SLOG(SLOG_DEBUG, "\t'%s' : '%s'", kv->key, kv->value);
            kv = kv->next;
        }
    }
}

struct connection {
	int fd;
	int proto;

	struct connection *prev;
	struct connection *next;

	struct event *ev_read;
	struct event *ev_write;
	
	struct evbuffer *evbin;
	struct evbuffer *evbout;

    struct request *request;
    struct rtsp_session *session;
	struct rtsp_server *server;
	
	struct sockaddr_in client_addr;
};

/* internal function prototype */
static void rtsp_server_connect_cb(struct evconnlistener *listener, 
	evutil_socket_t fd, struct sockaddr *sin, int socklen, void *arg);
static void rtsp_server_connect_error_cb(struct evconnlistener *listener, void *arg);
static void rtsp_server_add_connection(struct rtsp_server *server, struct connection *conn);
static void rtsp_server_del_connection(struct rtsp_server *server, struct connection *conn);

struct connection *connection_new(struct rtsp_server *server, int fd, struct sockaddr *sin);
void connection_free(struct connection *conn);
static void connection_default_write_cb(evutil_socket_t fd, short events, void *arg);
static void connection_default_read_cb(evutil_socket_t fd, short event, void *arg);

/* function declaration */
struct connection *
connection_new(struct rtsp_server *server, int fd, struct sockaddr *sin)
{
    struct connection *conn = NULL;

    /* set socket TCP_NODELAY */
    int i = 1;
    if (setsockopt(fd, SOL_TCP, TCP_NODELAY, &i, sizeof(i)) < 0 ) {
        SLOG(SLOG_ERROR, "faied to setting TCP_NODELAY on socket %d", fd);
    }
    
    conn = (struct connection *)calloc(1, sizeof(struct connection));
    if (!conn) {
        SLOG(SLOG_ERROR, "failed to alloc new connection on socket %d", fd);
        return NULL;
    }

    conn->fd = fd;
    conn->proto = CONNECTION_PROTO_RAW_TCP;
    conn->prev = conn;
    conn->next = conn;
    conn->server = server;
    memcpy(&(conn->client_addr), sin, sizeof(struct sockaddr_in));

    conn->ev_read = event_new(server->base, fd, EV_READ | EV_PERSIST, connection_default_read_cb, conn);
    if (!conn->ev_read) {
        SLOG(SLOG_ERROR, "failed to create read event for connection on socket %d", fd);
        goto failed;
    }

    conn->ev_write = event_new(server->base, fd, EV_WRITE, connection_default_write_cb, conn);
    if (!conn->ev_write) {
        SLOG(SLOG_ERROR, "failed to create write event for connection on socket %d", fd);
        goto failed;
    }

    conn->evbin = evbuffer_new();
    if (!conn->evbin) {
        SLOG(SLOG_ERROR, "failed to create input evbuffer for connection on socket %d", fd);
        goto failed;
    }

    conn->evbout = evbuffer_new();
    if (!conn->evbout) {
        SLOG(SLOG_ERROR, "failed to create output evbuffer for connection on socket %d", fd);
        goto failed;
    }

    SLOG(SLOG_DEBUG, "create new connection on socket %d", fd);

    /* add read event to eventloop and trigger it if no data after 10s */
    struct timeval timeout = {10, 0};
    event_add(conn->ev_read, &timeout);

    return conn;
    
failed:
    connection_free(conn);
    return NULL;
}

void 
connection_free(struct connection *conn)
{
    if (conn) {
        if (conn->fd)
            close(conn->fd);
        if (conn->evbout)
            evbuffer_free(conn->evbout);
        if (conn->evbin)
            evbuffer_free(conn->evbin);
        if (conn->ev_write)
            event_free(conn->ev_write);
        if (conn->ev_read)
            event_free(conn->ev_read);
        free(conn);
    }
}

static int 
connection_parse_protocol(struct connection *conn)
{
    struct evbuffer_ptr it;
    char firstline[512];
    char *lastspace = NULL;

    /* copy out firstline */
    it = evbuffer_search_eol(conn->evbin, NULL, NULL, EVBUFFER_EOL_ANY);
    if (it.pos < 0) {
        goto failed;
    }
    if (it.pos >= sizeof(firstline)) {
        it.pos = sizeof(firstline) - 1;
    }
    evbuffer_copyout(conn->evbin, &firstline, it.pos);
    firstline[it.pos] = '\0';
    
    SLOG(SLOG_DEBUG, "firstline %d: '%s'", it.pos, firstline);

    lastspace = strrchr(firstline, ' ');
    if (!lastspace) {
        SLOG(SLOG_ERROR, "error find proto field in firstline: '%s'", firstline);
        goto failed;
    }

    if (strncmp(lastspace + 1, "RTSP/", 5) == 0) {
        conn->proto = CONNECTION_PROTO_RTSP;
        SLOG(SLOG_DEBUG, "connection protol: RTSP");
    } else if (strncmp(lastspace + 1, "HTTP/", 5) == 0) {
        conn->proto = CONNECTION_PROTO_HTTP;
        SLOG(SLOG_DEBUG, "connection protol: HTTP");
    } else if (strncmp(lastspace + 1, "SIP/", 4) == 0) {
        conn->proto = CONNECTION_PROTO_SIP;
        SLOG(SLOG_DEBUG, "connection protol: SIP");
    } else {
        SLOG(SLOG_ERROR, "error parse proto in firstline: '%s'", firstline);
        goto failed;
    }
    
    return 0;
    
failed:
    return -1;
}


static int 
connection_parse_firstline(struct connection *conn, struct request *req)
{
	int ret = 0;
	size_t line_length = 0;
	
	char *line = NULL;
	char *token = NULL;

    char *tmp = NULL;

	char *method = NULL;
	char *uri = NULL;
	char *protocol = NULL;
	char *protocol_version = NULL;
    char *host = NULL;
    char *port = NULL;

	line = evbuffer_readln(conn->evbin, &line_length, EVBUFFER_EOL_ANY);
	if (line == NULL) {
		SLOG(SLOG_ERROR, "Could not read first line");
		goto failed;
	}
	if (line_length > 2048) {
		SLOG(SLOG_ERROR, "firstline too long(len > 2048)");
		goto failed;
	}

    token = line;
	method = strsep(&token, " ");
	if (token == NULL || method != line) {
	    SLOG(SLOG_ERROR, "bad request, request method illegal");
	    goto failed;
	}

	tmp = strsep(&token, " ");
	if (token == NULL) {
	    SLOG(SLOG_ERROR, "bad request, request_uri illegal");
		goto failed;
    }

    protocol_version = strsep(&token, " ");
    if (token != NULL) {
        SLOG(SLOG_ERROR, "bad request, protocol and version illegal");
        goto failed;
    }
    
    if (*tmp == '/') { 
        uri = tmp + 1;
    } else {
        protocol = tmp;
        while (*tmp != ':' && *tmp != '\0') tmp++;
        if (*tmp == ':') {
            *tmp++ = '\0';
            while (*tmp == '/') tmp++;
            host = tmp;
            while (*tmp != '/' && *tmp != ':' && *tmp != '\0') tmp++;
            if (*tmp == '/') {
                *tmp++ = '\0';
                uri = tmp;
            } else if (*tmp == ':') {
                *tmp++ = '\0';
                port = tmp;
                while (*tmp != '/' && *tmp != '\0') tmp++;
                if (*tmp == '/') {
                    *tmp++ = '\0';
                    uri = tmp;
                } else {
                    uri = tmp;
                }
            } else {
                goto failed;
            }
        } else {
            goto failed;
        }
    }

    tmp = uri;
    while (*tmp != '\0' && *tmp != '?') tmp++;
    if (*tmp == '?') {
        *tmp = '\0';
        tmp++;
        if (*tmp != '\0') {
            request_parse_param(req, tmp);
        }
    }

    req->method = method;   /* point to line, should be free when request finish */
    req->host = host;
    req->port = port;
    req->uri = uri;
    req->protocol = protocol;
    req->protocol_version = protocol_version;

    request_dump(req);

	return 0;
failed:
	if (line) {
		free(line);
	}
	return -1;
}

static int 
connection_parse_header(struct connection *conn, struct request *req)
{
    int ret = 0;
    char *line = NULL;

    // TODO: line can be used before a request end?(reduce malloc&free count)
    while ((line = evbuffer_readln(conn->evbin, NULL, EVBUFFER_EOL_ANY))
           != NULL) {
        char *key = NULL;
        char *value = NULL;

        /* Last header */
        if (*line == '\0') {
            free(line);
            break;
        }

        /* extract header name and value */
        value = line;
        key = strsep(&value, ":");
        if (value == NULL)
            goto failed;

        value += strspn(value, " ");

        /* add to request header */
        struct key_value *kv = NULL;
        kv = key_value_new(key, value);
        key_value_list_add(&req->header, kv);

        free(line);
    }
    
    return 0;
failed:
    if (line)
        free(line);
    return -1;
}

/**
 * 
 * return: 0 if succeed, 1 if need more data, -1 if error
 */
static int 
connection_parse_request(struct connection *conn)
{
    struct request *req = NULL;
    int ret = 0;

    if (conn->proto == CONNECTION_PROTO_RTSP) {
        char first_char = 0;

        evbuffer_copyout(conn->evbin, &first_char, 1);
        if (first_char == '$') {
            /* interleave receive */
            // TODO: parse interleave rtsp request (rtcp)
        }
    }
    
    /* wait until a "\r\n\r\n" is found */
    if (evbuffer_find(conn->evbin, (const unsigned char *)"\r\n\r\n", 4) == NULL) {
       if (evbuffer_get_length(conn->evbin) < MAX_REQUEST_HEADER_LENGTH) {
           /* wait for more data */
           return 1;
       } else {
           /* request header large than 4096 bytes, bad request */
           // TODO: response bad request and drain evbin buffer
           goto failed;
       }
    }

    req = request_new();
    if (!req) {
        goto failed;
    }
    ret = connection_parse_firstline(conn, req);
	if (ret) {
	    goto failed;
	}

    ret = connection_parse_header(conn, req);
    if (ret) {
        goto failed;
    }

	return 0;
failed: 
    if (req)
        request_free(req);
    return -1;
}

static void
connection_default_read_cb(evutil_socket_t fd, short event, void *arg)
{
    struct connection *conn = (struct connection *)arg;
    int ret = 0;

    /* handle read timeout */
    if (event & EV_TIMEOUT) {
        // TODO: use this to do session timeout?
        SLOG(SLOG_DEBUG, "connection timeout on socket %d", fd);
        goto failed;
    } else if (event & EV_READ) {
        ret = evbuffer_read(conn->evbin, fd, MAX_REQUEST_HEADER_LENGTH);
        if (ret < 0 || ret == 0) {
            SLOG(SLOG_DEBUG, "connection error %d, end connection", fd);
            goto failed;
        }
        
        if (conn->proto == CONNECTION_PROTO_RAW_TCP) {
            /* wait until a "\r\n\r\n" is found */
            if (evbuffer_find(conn->evbin, (const unsigned char *)"\r\n\r\n", 4) == NULL) {
                if (evbuffer_get_length(conn->evbin) < MAX_REQUEST_HEADER_LENGTH) {
                    /* wait for more data */
                    return;
                } else {
                    /* request header large than 4096 bytes, bad request */
                    // TODO: response bad request and drain evbin buffer
                    goto failed;
                }
            }

            ret = connection_parse_protocol(conn);
            if (ret) {
                SLOG(SLOG_DEBUG, "connection parse protocol failed");
                goto failed;
            }
        }

        if ((ret = connection_parse_request(conn))) {
            if (ret == 1)
                return;
            else
                goto failed;
        }

        // TODO: 
        return;
    }
failed:
    // TODO: response bad request and drain evbin buffer 
    // TODO: end session?

    rtsp_server_del_connection(conn->server, conn);
    connection_free(conn);
}

static void 
connection_default_write_cb(evutil_socket_t fd, short events, void *arg)
{
    ;
}

static void
rtsp_server_add_connection(struct rtsp_server *server, struct connection *conn)
{
    struct connection *head = server->conn_head;

    conn->next = head->next;
    conn->prev = head;
    head->next->prev = conn;
    head->next = conn;
    server->n_connections++;

    SLOG(SLOG_DEBUG, "add connection to server");
    
    if (conn->proto == CONNECTION_PROTO_RAW_TCP) {
        
    }

    // TODO: need do extra stuff if proto is rtsp or http?
}
static void
rtsp_server_del_connection(struct rtsp_server *server, struct connection *conn)
{
    conn->prev->next = conn->next;
    conn->next->prev = conn->prev;
    conn->prev = conn;
    conn->next = conn;
    server->n_connections--;
    
    if (conn->proto == CONNECTION_PROTO_RAW_TCP) {
        
    }

    SLOG(SLOG_DEBUG, "delete connection from server");
    SLOG(SLOG_DEBUG, "");
    
    // TODO: do extra clean if proto is rtsp or http?
}

/*
 *	
 * */
static void
rtsp_server_connect_cb(struct evconnlistener *listener, 
	evutil_socket_t fd, struct sockaddr *sin, int socklen, void *arg)
{
	SLOG(SLOG_DEBUG, "new connection connect from %s on socket %d", 
	         inet_ntoa(((struct sockaddr_in *)sin)->sin_addr), fd);
	
	struct rtsp_server *server = (struct rtsp_server *)arg;

	struct connection *conn = connection_new(server, fd, sin);
    if (conn) {
        rtsp_server_add_connection(server, conn);
    }
}

/*
 *
 * */
static void
rtsp_server_connect_error_cb(struct evconnlistener *listener, void *arg)
{
	SLOG(SLOG_DEBUG, "connect error on listener");
}

/*
 * create a rtsp server defined in rtsp_server_param(dynamic alloc)
 * support features:
 * 		RTSP over TCP, RTP/RTCP over UDP.
 * 		RTP/RTCP over RTSP over TCP.
 *		RTP/RTCP/RTSP over HTTP over TCP.
 *		SIP
 *		Multicast
 *		Video Conference
 * */
struct rtsp_server * 
rtsp_server_new(struct event_base *base, struct rtsp_server_param *param)
{
	if (!param || !base) {
		return NULL;
	}
	
	struct rtsp_server *server = NULL;
	struct rtsp_server_param *new_param = NULL;
	
	server = calloc(1, sizeof(struct rtsp_server));
	if (!server) {
		SLOG(SLOG_ERROR, "failed to alloc rtsp server");
		goto failed;
	}
	server->base = base;

	new_param = calloc(1, sizeof(struct rtsp_server_param));
	if (!new_param) {
		SLOG(SLOG_ERROR, "failed to alloc rtsp server param");
		goto failed;
	}
	*new_param = *param;
	server->param = new_param;
	
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(param->port);
	sin.sin_addr.s_addr = INADDR_ANY;

	/* listen on rtsp port */
	server->listener = evconnlistener_new_bind(server->base, 
									rtsp_server_connect_cb, 
									server, 
									LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_EXEC, 
									100, 
									(struct sockaddr *)&sin, 
									sizeof(sin));
	if (!server->listener) {
		SLOG(SLOG_ERROR, "failed to create listener on port %d", param->port);
		goto failed;
	}
	evconnlistener_set_error_cb(server->listener, 
								rtsp_server_connect_error_cb);

	/* if enable http tunnel, then listen on http port */
	if (param->enable_http_tunnel) {
		sin.sin_family = AF_INET;
		sin.sin_port = htons(param->http_tunnel_port);
		sin.sin_addr.s_addr = INADDR_ANY;
		
		server->http_listener = evconnlistener_new_bind(server->base, 
									rtsp_server_connect_cb, 
									server, 
									LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_EXEC, 
									100, 
									(struct sockaddr *)&sin, 
									sizeof(sin));
		if (!server->http_listener) {
			SLOG(SLOG_ERROR, "failed to create listener on port %d", param->http_tunnel_port);
			goto failed;
		}
		evconnlistener_set_error_cb(server->http_listener, 
									rtsp_server_connect_error_cb);
	}

    /* only prev and next is used in head node, other data fields are ignored */
	server->conn_head = calloc(1, sizeof(struct connection));
	if (!server->conn_head) {
	    SLOG(SLOG_ERROR, "failed to create connection head in rtsp server");
	    goto failed;
	}
	server->conn_head->next = server->conn_head;
	server->conn_head->prev = server->conn_head;

    /* only prev and next is used in head node, other data fields are ignored */
	server->session_head = calloc(1, sizeof(struct rtsp_session));
	if (!server->session_head) {
	    SLOG(SLOG_ERROR, "failed to create session head in rtsp server");
	    goto failed;
	}
	server->session_head->next = server->session_head;
	server->session_head->prev = server->session_head;

	return server;
	
failed:
	rtsp_server_free(server);
	return NULL;
}

void
rtsp_server_free(struct rtsp_server *server)
{
	if (server) {
	    if (server->session_head)
	        free(server->session_head);
        if (server->conn_head)
	        free(server->conn_head);
		if (server->http_listener)
			evconnlistener_free(server->http_listener);
		if (server->listener)
			evconnlistener_free(server->listener);
		if (server->n_sessions > 0) {
			// TODO: free sessions
		}
		if (server->param)
			free(server->param);
		free(server);
	}
}

