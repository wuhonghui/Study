#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/buffer.h"
#include "event2/listener.h"
#include "event2/buffer_compat.h"

#include "common.h"
#include "httplib.h"
#include "httplib-internal.h"
#include "http-handler.h"

#define TIMEBUF_LEN (64)

/* Function declare */
static int http_server_add_connection(struct http_server *hs,
	struct http_connection *hc);

static int http_server_del_connection(struct http_server *hs,
	struct http_connection *hc);

static struct http_connection * http_connection_new(struct http_server *hs,
	int fd, struct sockaddr *sin);

static void http_uris_init(struct http_server *hs);

time_t monotonic_time(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
		return ts.tv_sec;
	return time(NULL);
}

long monotonic_time_nsec(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
		return ts.tv_nsec;
	return 0;
}

char * http_server_fmttime()
{
	static char server_fmttime[TIMEBUF_LEN];
	struct tm   *tm;
	time_t t = time(NULL);
	tm=localtime(&t);
	strftime(server_fmttime, TIMEBUF_LEN, "%Y-%m-%d %H:%M:%S %Z", tm);
	return server_fmttime;
}

/* Function definition */
static char* http_strcpy(char * to, char * from)
{
	char * src;
	char * dest;

	if (to == from)
	    return to;

	if (to < from) {    /* copy from left to right, shorten string */
	    src  = from;
	    dest = to;
	    while ( (*dest = *src) != 0) {
	        dest++;
	        src++;
	    }
	}else{            	/* copy from right to left, lengthen string */
	    src  = from + strlen (from);
	    dest = to + strlen (from);
	    while (dest >= from) {
	        *dest = *src;
	        dest--;
	        src--;
	    }
	}
	return to;
}

static int http_line_unescape(char * string)
{
    unsigned long length;
    unsigned long i = 0;
    char          buffer[3];
    char          tmp;

    if (string == NULL)
    	return (0);

    length = strlen (string);

    while (i < length) {
	    if (string[i] == '+')
	        string[i] = ' ';                    	/* replace '+' by spaces */
	    if ((string[i] == '%') && (i < length-2)) {
	        if (isxdigit ((int) string[i+1]) && isxdigit ((int) string[i+2])) {
	            strncpy (buffer, &(string[i+1]), 2);
	            buffer[2] = 0;
	            tmp = (char) strtol (buffer, NULL, 16);
	            if (tmp != 0) {
	                http_strcpy (&(string [i]),  	/* move string 2 chars   */
	                            &(string[i+2]));
	                string[i] = tmp;       			/* replace % by new char */
	                length -= 2;
                }
            }
	    }
	    i++;
    }
    return (0);
}

static int http_parse_param(struct http_connection *hc, char *str)
{
    char * szParamPtr;
    char * szParamValue;
	struct name_value * parm;

	while ((szParamValue = strtok_r(str, "&", &szParamPtr)) != NULL) {
		char * szToken = NULL;
        char * szName = NULL;
        char * szValue = NULL;

		szName = szParamValue;
		szToken = strchr(szParamValue, '=');
		if( szToken != NULL ) {
			*szToken = '\0';
			szValue = szToken + 1;
		}

        /* szValue szName needs to be unescaped here */
        http_line_unescape(szName);
		http_line_unescape(szValue);
		if (szName != NULL && szValue != NULL && strlen (szName) > 0) {
			parm = (struct name_value *)malloc(sizeof(struct name_value));
			if (parm) {
				parm->name = szName;
				parm->value = szValue;

				//dbprintf("name:'%s' value:'%s'\n", parm->name, parm->value);
				parm->next = hc->query_params;
				hc->query_params = parm;
			}else {
				return -1;
			}
        }
		str = NULL;
	}

	return 0;
}

static int http_parse_request_line(struct http_connection *hc, char *line)
{
	char *method;
	char *uri;
	char *version;

	method = strsep(&line, " ");
	if (line == NULL)
		goto failed;
	uri = strsep(&line, " ");
	if (line == NULL)
		goto failed;
	version = strsep(&line, " ");
	if (line != NULL)
		goto failed;

	if (strcmp(method, "GET") == 0) {
		hc->method = "GET";
	} else if (strcmp(method, "POST") == 0) {
		hc->method = "POST";
	} else if (strcmp(method, "HEAD") == 0) {
		hc->method = "HEAD";
	} else if (strcmp(method, "PUT") == 0) {
		hc->method = "PUT";
	} else if (strcmp(method, "DELETE") == 0) {
		hc->method = "DELETE";
	} else if (strcmp(method, "OPTIONS") == 0) {
		hc->method = "OPTIONS";
	} else if (strcmp(method, "TRACE") == 0) {
		hc->method = "TRACE";
	} else {
		dbprintf("Unknown http request type %s\n", method);
		goto failed;
	}

	int major, minor;
	char ch;
	int n = sscanf(version, "HTTP/%d.%d%c", &major, &minor, &ch);
	if (n != 2 || major > 1) {
		goto failed;
	}
	hc->proto_major = major;
	hc->proto_minor = minor;

	if ((hc->uri = strdup(uri)) == NULL) {
		goto failed;
	}

	if (hc->uri) {
		char *query = hc->uri;
		while(*query != '\0' && *query != '?') {
			query++;
		}

		if (*query == '?') {
			*query = '\0';
			query++;
			if (query != NULL) {
				http_parse_param(hc, query);
			}
		}
	}
	return 0;
failed:
	return -1;
}

static int http_parse_first_line(struct http_connection *hc, struct evbuffer *evb)
{
	int ret = 0;
	char *line = NULL;
	size_t line_length = 0;

	line = evbuffer_readln(evb, &line_length, EVBUFFER_EOL_CRLF);
	if (line == NULL) {
		dbprintf("Could not read http first line\n");
		return -1;
	}
	if (line_length > 2048) {
		dbprintf("Http firstline too long(len > 2048)\n");
		goto failed;
	}

	ret = http_parse_request_line(hc, line);
	if (ret) {
		dbprintf("Could not parse http request line\n");
		goto failed;
	}

	free(line);

	return 0;
failed:
	if (line) {
		free(line);
	}
	return -1;
}

static int http_parse_headers(struct http_connection *hc, struct evbuffer *evb)
{
	int ret = 0;
	char *line = NULL;
	size_t line_length = 0;
	struct name_value * parm = NULL;

	while ((line = evbuffer_readln(evb, &line_length, EVBUFFER_EOL_CRLF))
	       != NULL) {
		char *skey, *svalue;

		if (*line == '\0') { /* Last header - Done */
			free(line);
			break;
		}

		/* Processing of header lines */
		svalue = line;
		skey = strsep(&svalue, ":");
		if (svalue == NULL)
			goto failed;

		svalue += strspn(svalue, " ");
		//dbprintf("header name:'%s' header value:'%s'\n", skey, svalue);

		parm = (struct name_value *)malloc(sizeof(struct name_value));
		if (!parm) {
			goto failed;
		}

		parm->name = strdup(skey);
		parm->value = strdup(svalue);

		parm->next = hc->request_header;
		hc->request_header = parm;

		free(line);
	}

	return 0;
failed:
	if (line) {
		free(line);
	}
	return -1;
}

char * http_get_query_param(struct http_connection *hc, const char *name)
{
	struct name_value * parm = hc->query_params;
	while(parm != NULL) {
		if (!strcmp(name, parm->name)) {
			return parm->value;
		}
		parm = parm->next;
	}

	return NULL;
}

char * http_get_request_header(struct http_connection *hc, const char *name)
{
	struct name_value * parm = hc->request_header;
	while(parm != NULL) {
		if (!strcmp(name, parm->name)) {
			return parm->value;
		}
		parm = parm->next;
	}

	return NULL;
}

int http_header_add(struct http_connection *hc, char *header)
{
	evbuffer_add_printf(hc->evbout, "%s\r\n", header);
	return 0;
}

int http_header_start(struct http_connection *hc,
	int status, char *result, char *type)
{
	struct tm	*tm;
	time_t		t;
	char buf[64];

	/* Wed, 17 May 2006 17:33:00 GMT */
	t=time(NULL);
	tm=gmtime(&t);
	strftime(buf, 64, "%a, %d %b %Y %H:%M:%S %Z", tm);

	hc->status = status;
	evbuffer_add_printf(hc->evbout, "HTTP/1.0 %d %s\r\n", status, result);
	evbuffer_add_printf(hc->evbout, "Server: Streamd\r\n");
	evbuffer_add_printf(hc->evbout, "Date: %s\r\n", buf);
	if (type)
		evbuffer_add_printf(hc->evbout, "Content-Type: %s\r\n", type);

	return 0;
}

int http_header_clength(struct http_connection *hc, ssize_t length)
{
	evbuffer_add_printf(hc->evbout, "Content-Length: %d\r\n", length);
	return 0;
}

int http_header_end(struct http_connection *hc)
{
	evbuffer_add_printf(hc->evbout, "Connection: close\r\n");
	evbuffer_add_printf(hc->evbout, "\r\n");
	return 0;
}

int http_evbuffer_writeout(struct http_connection *hc, int can_skip)
{
    int ret = 0;
    int origin_length = evbuffer_get_length(hc->evbout);
    int length = origin_length;
    int retry = 20;

    while((length = evbuffer_get_length(hc->evbout)) > 0 && retry--) {
        ret = evbuffer_write(hc->evbout, hc->fd);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                if (length == origin_length && can_skip) {
                    dbprintf("connection %p evbuffer_write error %d,"
						"stop write and drain buffer\n", hc, errno);
                    evbuffer_drain(hc->evbout, -1);

					/* treat as a error occur if failed many times consecutively */
					if (hc->consecutive_failed++ > 90) {
						dbprintf("connection %p have too many egain, stop it.\n",
							hc);
						return -1;
					} else {
                    	return 0;
					}
                } else {
                    continue;
                }
            } else if (errno == ENOENT){
                /* unknown error, but can be continue */
                dbprintf("connection %p evbuffer_write error %d,"
                	"continue\n", hc, errno);
                continue;
            } else {
                printf("connection %p evbuffer_write error %d, "
					"stop write and end connection\n", hc, errno);
                return -1;
            }
        }
    }

    length = evbuffer_get_length(hc->evbout);
	if (length == 0) {
		hc->consecutive_failed = 0;
	}

    return origin_length - length;
}

static struct http_uri * http_uri_new(const char *uri,
	void (*handler_cb)(evutil_socket_t fd, short events, void *arg))
{
	struct http_uri *hu = (struct http_uri *)malloc(sizeof(struct http_uri));
	if (!hu) {
		printf("Could not create new http uri\n");
		return NULL;
	}

	hu->uri = strdup(uri);
	if (!hu->uri) {
		printf("Could not create new uri buf\n");
		free(hu);
		return NULL;
	}
	hu->handler_cb = handler_cb;
	hu->next = NULL;

	return hu;
}

static void http_uri_free(struct http_uri *hu)
{
	if (hu->uri)
		free(hu->uri);
	if (hu)
		free(hu);
}

static int http_register_uri(struct http_server *hs, const char *uri,
	void (*handler_cb)(evutil_socket_t fd, short events, void *arg))
{
	struct http_uri *hu = http_uri_new(uri, handler_cb);
	if (!hu) {
		printf("Could not register uri %s\n", uri);
		return -1;
	}

	struct http_uri *prev = NULL;
	if (hs->uris == NULL) {
		hs->uris = hu;
	} else {
		prev = hs->uris;
		while (prev->next != NULL) {
			prev = prev->next;
		}
		prev->next = hu;
	}

	return 0;
}

static int http_dispatch_uri_handler_cb(struct http_connection *hc)
{
	if (!hc || !hc->hs || !hc->uri)
		return -1;

	struct http_uri *hu = hc->hs->uris;

	while(hu != NULL) {
		if (strcmp(hu->uri, hc->uri) == 0) {
			break;
		} else {
			hu = hu->next;
		}
	}

	if (hu && hu->handler_cb) {
		event_assign(hc->ev_write, hc->hs->base, hc->fd, EV_WRITE, hu->handler_cb, hc);
	} else {
		event_assign(hc->ev_write, hc->hs->base, hc->fd, EV_WRITE, http_404_cb, hc);
	}

	event_add(hc->ev_write, NULL);

	return 0;
}

static void http_read_cb(evutil_socket_t fd, short events, void *arg)
{
	int ret = 0;
	struct http_connection *hc = arg;

	if (events & EV_TIMEOUT) {
		printf("timeout on fd %d, hc %p\n", fd, hc);
		goto failed;
	}

	if (events & EV_READ) {
		ret = evbuffer_read(hc->evbin, fd, 4096);
		if (ret == -1 || ret == 0) {
			goto failed;
		}

		if (hc->connection_status == HTTP_CONNECTION_STATUS_READ_HEADER) {
			if (evbuffer_find(hc->evbin, (const unsigned char *)"\r\n\r\n", 4) == NULL &&
				evbuffer_get_length(hc->evbin) < 4096)
			{
				// wait to read more data
				return;
			} else {
				//dbprintf("evbuffer_get_length: %d\n", evbuffer_get_length(hc->evbin));
			}

			ret = http_parse_first_line(hc, hc->evbin);
			if (ret) {
				goto failed;
			}

			ret = http_parse_headers(hc, hc->evbin);
			if (ret) {
				goto failed;
			}

			/* set no-timeout read */
			event_del(hc->ev_read);
			event_add(hc->ev_read, NULL);
			hc->connection_status = HTTP_CONNECTION_STATUS_READ_BODY;
		} else if (hc->connection_status == HTTP_CONNECTION_STATUS_READ_BODY) {

		}

		/* dispatch to handler */
		ret = http_dispatch_uri_handler_cb(hc);
		if (ret) {
			goto failed;
		}
	}

	return;
failed:
	http_connection_free(hc);
}

static void http_connect_cb(struct evconnlistener *listener,
	evutil_socket_t fd, struct sockaddr *sin,
	int socklen, void *arg)
{
	int ret = 0;
	struct http_server *hs = arg;

	struct http_connection *hc  = http_connection_new(hs, fd, sin);
	if (!hc) {
		printf("Could not accept a new connection from %s\n",
			inet_ntoa(((struct sockaddr_in *)sin)->sin_addr));
		return;
	}

	http_server_add_connection(hs, hc);
}

static void http_connect_error_cb(struct evconnlistener *listener, void *arg)
{
	printf("evconnlistenner connect error\n");
}

static struct http_connection * http_connection_new(struct http_server *hs,
		int fd, struct sockaddr *sin)
{
	struct http_connection *hc = NULL;
	hc = (struct http_connection *)malloc(sizeof(struct http_connection));
	if (!hc) {
		printf("Could not create new http connection\n");
		return NULL;
	}

	memset(hc, 0, sizeof(struct http_connection));

	hc->hs = hs;
	hc->fd = fd;
	memcpy(&(hc->sin), sin, sizeof(struct sockaddr_in));
	hc->connection_status = HTTP_CONNECTION_STATUS_READ_HEADER;
	hc->status = HTTP_OK;

	if (fd >= 0) {	/* init head node with fd<0 */
		/* create a event for this fd */
		hc->ev_read = event_new(hs->base, fd, EV_READ | EV_PERSIST, http_read_cb, hc);
		if (!hc->ev_read) {
			printf("Could not create event for http connection(fd %d)\n", fd);
			goto failed;
		}

		hc->ev_write = event_new(hs->base, fd, EV_WRITE, http_404_cb, hc);
		if (!hc->ev_write) {
			printf("Could not create event for http connection(fd %d)\n", fd);
			goto failed;
		}

		/* create a event buffer for read data */
		hc->evbin = evbuffer_new();
		if (!hc->evbin) {
			printf("Could not create input evbuffer for http connection(fd %d)\n", fd);
			goto failed;
		}

		/* create a event buffer for read data */
		hc->evbout = evbuffer_new();
		if (!hc->evbout) {
			printf("Could not create output evbuffer for http connection(fd %d)\n", fd);
			goto failed;
		}

		int i = 1;
		setsockopt(hc->fd, SOL_TCP, TCP_NODELAY, &i, sizeof(i));

		/* close connection if not data received in 3 seconds */
		struct timeval timeout = {10, 0};
		event_add(hc->ev_read, &timeout);
	}

	/* init list node */
	hc->prev = hc;
	hc->next = hc;

	dbprintf("http connection new %p\n", hc);

	return hc;
failed:
	if (hc->evbout)
		evbuffer_free(hc->evbout);
	if (hc->evbin)
		evbuffer_free(hc->evbin);
	if (hc->ev_write)
		event_free(hc->ev_write);
	if (hc->ev_read)
		event_free(hc->ev_read);
	if (hc)
		free(hc);
	return NULL;
}

void http_connection_free(struct http_connection *hc)
{
	if (!hc)
		return;

	dbprintf("http connection free %p %s.\n", hc, hc->uri);

	if (hc->evbout) {
		evbuffer_free(hc->evbin);
	}
	if (hc->evbin) {
		evbuffer_free(hc->evbout);
	}
	if (hc->ev_read) {
		event_free(hc->ev_read);
	}
	if (hc->ev_write) {
		event_free(hc->ev_write);
	}
	if (hc->fd) {
		close(hc->fd);
	}

	if (hc->priv) {
		if (hc->free_priv) {
			hc->free_priv(hc->priv);
		}
	}

	/* delete self from dlist */
	http_server_del_connection(hc->hs, hc);

	/* delete http resources */
	if (hc->uri) {
		free(hc->uri);
	}

	if (hc->username) {
		free(hc->username);
	}

	/* delete request header */
	struct name_value *nv= NULL;
	if (hc->request_header) {
		while(hc->request_header != NULL) {
			nv = hc->request_header;
			hc->request_header = nv->next;
			if (nv->name)
				free(nv->name);
			if (nv->value)
				free(nv->value);
			free(nv);
		}
	}

	/* delete query params */
	if (hc->query_params) {
		while(hc->request_header != NULL) {
			nv = hc->request_header;
			hc->request_header = nv->next;
			/* name and value pointer is point to somewhere in hc->uri */
			free(nv);
		}
	}

	if (hc)
		free(hc);

	return;
}

static int http_server_add_connection(struct http_server *hs,
	struct http_connection *hc)
{
	hc->next = hs->hc->next;
	hc->prev = hs->hc;
	hs->hc->next->prev = hc;
	hs->hc->next = hc;

	return 0;
}

static int http_server_del_connection(struct http_server *hs,
	struct http_connection *hc)
{
	if (hc == hs->hc) {
		return -1;
	}

	hc->next->prev = hc->prev;
	hc->prev->next = hc->next;
	hc->next = hc;
	hc->prev = hc;

	return 0;
}

struct http_server * http_server_new(struct event_base *base, int port)
{
	struct http_server *hs = NULL;
	hs = (struct http_server *)malloc(sizeof(struct http_server));
	if (!hs) {
		printf("Could not create http server\n");
		return NULL;
	}
	memset(hs, 0, sizeof(struct http_server));

	hs->base = base;

	struct sockaddr_in sin;
	sin.sin_family=AF_INET;
	sin.sin_port=htons(port);
	sin.sin_addr.s_addr=INADDR_ANY;

	/* init connection listener */
	hs->listener = evconnlistener_new_bind(base, http_connect_cb, hs,
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_EXEC,
		100, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in));
	if (!hs->listener) {
		printf("Could not bind a listener to port %d\n", port);
		goto failed;
	}
	evconnlistener_set_error_cb(hs->listener, http_connect_error_cb);

	/* init connection head node, for convinents */
	hs->hc = http_connection_new(hs, -1, (struct sockaddr *)&sin);
	if (!hs->hc) {
		printf("Could not create a initial connection for http server\n");
		goto failed;
	}

	/* init uris */
	http_uris_init(hs);

	return hs;
failed:
	http_server_free(hs);
	return NULL;
}

void http_server_free(struct http_server *hs)
{
	if (hs) {
		if (hs->uris != NULL) {
			struct http_uri *hu = NULL;
			while (hs->uris != NULL) {
				hu = hs->uris;
				hs->uris = hu->next;
				http_uri_free(hu);
			}
		}

		if (hs->listener) {
			evconnlistener_free(hs->listener);
		}

		if (hs->hc) {
			struct http_connection *hc = hs->hc->next;
			if (hc) {
				while (hc != hs->hc) {
					dbprintf("http connections %p not free\n", hc);
					http_connection_free(hc);
					hc = hs->hc->next;
				}
				free(hs->hc);
			}
		}

		free(hs);
	}
}

/* add new http url handler here */
static void http_uris_init(struct http_server *hs)
{
	http_register_uri(hs, "/", http_gen_cb);
	http_register_uri(hs, "", http_gen_cb);
	http_register_uri(hs, "/stream/video/mjpeg", http_stream_video_mjpeg_cb);
	http_register_uri(hs, "/stream/audio/wavpcm", http_stream_audio_wavpcm_cb);
	http_register_uri(hs, "/stream/video/h264", http_stream_video_h264_cb);
	http_register_uri(hs, "/stream/video/snapshot.jpg", http_stream_video_snapshot_cb);
	http_register_uri(hs, "/stream/mixed/h264", NULL);
	http_register_uri(hs, "/stream/mixed/mjpeg", NULL);
}

