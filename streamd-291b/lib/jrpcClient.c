#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include "cJSON.h"
#include "jrpc.h"
#include "jrpcClient.h"

/* jrpc client error */
#define JRPC_JSON_ERR		-32500


#define JRPC_SOCK_INIT_ERR		-32503
#define JRPC_SOCK_CONNECT_ERR	-32504
#define JRPC_RECV_ERR			-32505
#define JRPC_WAIT_TIMEOUT		-32506
#define JRPC_SELECT_ERR			-32507

#define JRPC_RESPONSE_FORMAT_ERR 	-32509
#define JRPC_RESPONSE_CALLID_UNMATCH	-32510
#define JRPC_PROTOCAL_UNMATCH		-32511


#define JRPC_CONNECTION_CLOSE	-32514
#define JRPC_SOCK_BIND_ERR		-32515





enum jrpc_result_t_type{ NONE, INT, DOUBLE, STRING };

struct _jrpc_result_t
{
	int callid;
	int err;
	int type;	//as above
	union {
		int i;
		double d;
		char * s;
	};
};

int jrpc_get_result_int(const jrpc_result_t * result, int *address)
{
	if( result && result->err == 0 && result->type == DOUBLE )
	{
		if (address)
			*address = result->i;
		return 0;
	}
	return -1;
}

int jrpc_get_result_double(const jrpc_result_t * result, double *address)
{
	if( result && result->err == 0 && result->type == DOUBLE )
	{
		if (address)
			*address = result->d;
		return 0;
	}
	return -1;
}

const char * jrpc_get_result_string(const jrpc_result_t * result)
{
	if( result && result->err == 0 && result->type == STRING )
	{
		return result->s;
	}
	return NULL;
}

void jrpc_free_result(jrpc_result_t * result)

{
	if( result )
	{
		if( result->type == STRING )
			free(result->s);
		free(result);
	}
	result = NULL;
}


/**
 *	@fd			socket that connect to jrpc server
 *	@mode		specify the client's mode: UNIX or TCP; Long Connection or not; With Session or not
 *	@towait		time in seconds to wait for response from server
 *	@rset		use the select to perform timeout waitting
 *	@sessionid	dynamic allocated buffer
 *	@srv_in_addr	server address when in tcp socket mode
 *	@srv_un_addr	server address when in unix socket mode
 */
struct _jrpc_client_t {
	pthread_mutex_t fd_lock;
	int 	fd;
	int 	mode;
	int 	towait;
	char * 	sessionid;
	struct sockaddr_in srv_in_addr;
	struct sockaddr_un srv_un_addr;
};


static void _jrpc_close_connection(jrpc_client_t * client)
{
	if( !(client->mode & JRPC_CLIMODE_PERSIST) )
	{
		pthread_mutex_lock(&client->fd_lock);
		close(client->fd);
		shutdown(client->fd, SHUT_RDWR);
		client->fd = -1;
		pthread_mutex_unlock(&client->fd_lock);
	}
	return ;
}

static int _jrpc_create_socket(jrpc_client_t * client)
{
	if( client->mode & JRPC_CLIMODE_LOCALSOCKET )
	{
		if( -1 == (client->fd = socket(AF_LOCAL, SOCK_STREAM, 0)) )
		{
			return JRPC_SOCK_INIT_ERR;
		}

		int yes = 1;
		if ( -1 == setsockopt(client->fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) )
		{
			perror("setsockopt");
			return -1;
		}

		struct sockaddr_un client_addr;
		memset(&client_addr, '\0', sizeof client_addr);
		client_addr.sun_family = AF_LOCAL;
		strcpy(client_addr.sun_path, JRPC_CLIENT_LOCALSOCK_PATH);
		socklen_t clen = sizeof client_addr;

		unlink(JRPC_CLIENT_LOCALSOCK_PATH);
		if( -1 == bind(client->fd, (struct sockaddr*)&client_addr, clen) )
		{
			close(client->fd);
			shutdown(client->fd, SHUT_RDWR);
			client->fd = -1;
			return JRPC_SOCK_BIND_ERR;
		}
		return 0;
	}
	else
	{
		client->fd = socket(AF_INET, SOCK_STREAM, 0);
		return client->fd == -1 ? JRPC_SOCK_INIT_ERR : 0;
	}
}

static int _jrpc_nonblock_connect(jrpc_client_t * client, struct sockaddr* serv_addr, socklen_t slen)
{

	int flags = fcntl(client->fd, F_GETFL, 0);
	if( -1 == fcntl(client->fd, F_SETFL, flags|O_NONBLOCK) )
	{
		//block connect..
		printf("block connect\n");
		return connect(client->fd, serv_addr, slen);
	}

	int n = connect(client->fd, serv_addr, slen);
	if( n == 0 )
	{
		printf("connect completed immediately");
		fcntl(client->fd, F_SETFL, flags);
		return 0;
	}
	else if( n < 0 )
	{
		if(errno != EINPROGRESS)
			return -1;
		fd_set rset;
		fd_set wset;
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(client->fd, &rset);
		FD_SET(client->fd, &wset);
		struct timeval timeout = {client->towait, 500000};

		n = select(client->fd + 1, &rset, &wset, NULL, &timeout);
		if( n <= 0 )
		{
			printf("time out connect error");
			close(client->fd);
			client->fd = -1;
			return -1;
		}

		/*
			connect ok: writable
			connect err: writable and readable
			Note: Because we defined that server wouldn't sent data to client activily, so we
			could use the condiction "is socket readable?" to check if it was connected.
			but now we use another connect to check if the connection was built.
		*/
		int retval = -1;
		if( FD_ISSET(client->fd, &rset) && FD_ISSET(client->fd, &wset) )
		{
		/*	len = sizeof(error);
			if( 0 == getsockopt(client->fd, SOL_SOCKET, SO_ERROR, &error, &len) )
				return 0; */
			errno = 0;
			connect(client->fd, serv_addr, slen);
			retval = (errno == EISCONN ? 0 : -1);
			printf("both read wirte\n");
		}
		else if( !FD_ISSET(client->fd, &rset) && FD_ISSET(client->fd, &wset) )
		{
			retval = 0;
		}
		fcntl(client->fd, F_SETFL, flags);
		return retval;
	}
	return -1;
}

static int _jrpc_setup_connection(jrpc_client_t * client)
{
	int retval = -1;
	if( client->mode & JRPC_CLIMODE_LOCALSOCKET )
	{
		socklen_t sunlen = sizeof(client->srv_un_addr);
		retval = connect(client->fd, (struct sockaddr*)&client->srv_un_addr, sunlen) ? JRPC_SOCK_CONNECT_ERR : 0;
	}
	else
	{
		socklen_t sinlen = sizeof(client->srv_in_addr);

//		retval = connect(client->fd, (struct sockaddr*)&client->srv_in_addr, sinlen) ;//? JRPC_SOCK_CONNECT_ERR : 0;
		retval = _jrpc_nonblock_connect(client, (struct sockaddr*)&client->srv_in_addr, sinlen);

//		printf("errno %d\n", errno == ECONNREFUSED);
		retval = retval ? JRPC_SOCK_CONNECT_ERR : 0;
	}
	return retval;
}

static int _jrpc_get_sessionid(jrpc_client_t * client, char * name, char * token);

/*
 *	Success:	@return 0
 *	Error:		@return err(<0)
 */
static int _jrpc_reconnect(jrpc_client_t * client)
{
	int retval = 0;
	pthread_mutex_lock(&client->fd_lock);

	if( -1 == client->fd )
	{
		retval = _jrpc_create_socket(client);

		if( retval )
		{
			client->fd = -1;
			pthread_mutex_unlock(&client->fd_lock);
			return retval;
		}
		retval = _jrpc_setup_connection(client);
		if( retval )
		{
			close(client->fd);
			client->fd = -1;
			pthread_mutex_unlock(&client->fd_lock);
			return retval;
		}
		if( (client->mode & JRPC_CLIMODE_SESSION) && !client->sessionid )
		{
			retval = _jrpc_get_sessionid(client, "admin", "admin");
			if(retval)
			{
				close(client->fd);
				shutdown(client->fd, SHUT_RDWR);
				client->fd = -1;
			}
		}

	}

	pthread_mutex_unlock(&client->fd_lock);
	return retval;
}



static unsigned int _jrpc_gen_callid()
{
	//maybe need a lock?
	static unsigned int id = 1;
	return id++;
}

/**
 *	Problem: maybe need a better way to deal with the receive data.
 *	Now, when receive data from jrpcserver, need to check if the id of the data is equal
 *	to the param @callid. if unmatch return JRPC_RESPONSE_CALLID_UNMATCH and simply "drop" the data.
 *
 */
static int _jrpc_eval_result(jrpc_client_t * client, unsigned int callid, char * data, jrpc_result_t * result)
{
	cJSON * json = cJSON_Parse(data);
	if( !json )
	{
		printf("response not a json:%s\n", data);
		result->err = JRPC_RESPONSE_FORMAT_ERR;
		return JRPC_RESPONSE_FORMAT_ERR;
	}
	cJSON * jsonresult = cJSON_GetObjectItem(json, "result");
	cJSON * error = cJSON_GetObjectItem(json, "error");
	if( (jsonresult && error) || (!jsonresult && !error) )
	{
		cJSON_Delete(json);
		result->err = JRPC_PROTOCAL_UNMATCH;
		return JRPC_PROTOCAL_UNMATCH;
	}
	if( !jsonresult && error )
	{
		cJSON * errcode = cJSON_GetObjectItem(error, "code");
		if( errcode && errcode->type == cJSON_Number && errcode->valueint == JRPC_SESSION_TIMEOUT)
		{
			free(client->sessionid);
			client->sessionid = NULL;
			_jrpc_close_connection(client);
		}
		cJSON_Delete(json);
		result->err = JRPC_SESSION_TIMEOUT;
		return JRPC_SESSION_TIMEOUT;
	}
	if( jsonresult && !error )
	{
		cJSON * id = cJSON_GetObjectItem(json, "id");
		if( id && callid != id->valueint )
		{
			cJSON_Delete(json);
			result->err = JRPC_RESPONSE_CALLID_UNMATCH;
			return JRPC_RESPONSE_CALLID_UNMATCH;
		}
		if( jsonresult->type == cJSON_Number )
		{
			result->type = DOUBLE;
			result->d = jsonresult->valuedouble;
			result->i = jsonresult->valueint;
		}
		else if( jsonresult->type == cJSON_String )
		{
			result->type = STRING;
			int len = strlen(jsonresult->valuestring) + 1;
			result->s = malloc(len);
			if( !result )
			{
				result = NULL;
			}
			else
			{
				memset(result->s, '\0', len);
				strcpy(result->s, jsonresult->valuestring);
			}
		}
		else
			result->type = NONE;
	}
	cJSON_Delete(json);
	return 0;
}


static int _jrpc_get_response(jrpc_client_t * client, unsigned int callid, jrpc_result_t * result )
{
	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(client->fd, &rset);
	struct timeval timeout = {client->towait, 300000};

	int retval = -1;
	int ret = select( client->fd + 1, &rset, NULL, NULL, &timeout );
	switch( ret )
	{
	case 0 :
			{
				printf("call %d timeout\n", callid);
				result->err = JRPC_WAIT_TIMEOUT;
			}
			break;
	case -1:
			{
				printf("call %d error\n", callid);
				result->err = JRPC_SELECT_ERR;
			}
			break;
	default:
			{
				if( FD_ISSET(client->fd, &rset) )
				{
					char data[JRPC_PACKAGE_SIZE + 1];
					memset(data, '\0', sizeof data);
					char req[JRPC_PACKAGE_SIZE];
					memset(req, '\0', sizeof req);
					int pos = 0;
bk:
					do{
						if( pos >= JRPC_PACKAGE_SIZE )
						{
							pos = 0;
							memset(data, '\0', sizeof data);
						}
						int size = recv(client->fd, data+pos, JRPC_PACKAGE_SIZE - pos, 0);

						if( size < 0 )
						{
							printf("recv error\n");
							result->err = JRPC_RECV_ERR;
							close(client->fd);
							shutdown(client->fd, SHUT_RDWR);
							client->fd = -1;
							break;
						}
						else if( size == 0 )
						{
							printf("lost connect\n");
							result->err = JRPC_CONNECTION_CLOSE;
							close(client->fd);
							shutdown(client->fd, SHUT_RDWR);
							client->fd = -1;
							break;
						}
						pos += size;
						char * p = strstr(data, JRPC_DELIMITER);
						if( p )
						{
							strncpy(req, data, p - data);
							p += strlen(JRPC_DELIMITER);
							int plen = strlen(p);
							memmove(data, p, plen);
							memset(data+plen, '\0', sizeof data - plen);
							pos = strlen(data);
							retval = _jrpc_eval_result(client, callid, req, result);
						}
						else
							goto bk;
					}while(retval == JRPC_RESPONSE_CALLID_UNMATCH);
				}
			}break;
	}
	return retval;
}

static int _jrpc_send(int fd, char * json)
{
	int size = 0;
	do{
		size += send(fd, json + size, strlen(json) - size, MSG_NOSIGNAL);
		if( size == -1 )
			return -1;
	}while(size != strlen(json));
	return 0;
}


static int _jrpc_send_data(jrpc_client_t * client, char * json)
{
	int retval = 0;
	pthread_mutex_lock(&client->fd_lock);
	if( -1 == client->fd )
	{
		pthread_mutex_unlock(&client->fd_lock);
		return JRPC_CONNECTION_CLOSE;
	}
	retval = _jrpc_send(client->fd, json);
	if( -1 == retval )
	{
		close(client->fd);
		shutdown(client->fd, SHUT_RDWR);
		client->fd = -1;
		pthread_mutex_unlock(&client->fd_lock);
		return JRPC_CONNECTION_CLOSE;
	}
	retval = _jrpc_send(client->fd, JRPC_DELIMITER);
	if( -1 == retval )
	{
		close(client->fd);
		shutdown(client->fd, SHUT_RDWR);
		client->fd = -1;
		pthread_mutex_unlock(&client->fd_lock);
		return JRPC_CONNECTION_CLOSE;
	}
	pthread_mutex_unlock(&client->fd_lock);
	return 0;
}


static int _jrpc_get_sessionid(jrpc_client_t * client, char * name, char * token)
{
	int err = 0;
	/*	make json data	*/
	cJSON * root = cJSON_CreateObject();
	cJSON * params = cJSON_CreateArray();
	if( !root )
	{
		err = JRPC_JSON_ERR;
		return err;
	}
	if( !params )
	{
		cJSON_Delete(root);
		err = JRPC_JSON_ERR;
		return err;
	}
	unsigned int callid = _jrpc_gen_callid();
	cJSON_AddStringToObject(root, "jsonrpc", "2.0");
	cJSON_AddNumberToObject(root, "id", callid);
	cJSON_AddStringToObject(root, "method", "RPC_SessionAuth");
	cJSON_AddItemToArray(params, cJSON_CreateString(name));
	cJSON_AddItemToArray(params, cJSON_CreateString(token));
	cJSON_AddItemToObject(root, "params", params);

	char * json = cJSON_Print(root);
	cJSON_Delete(root);
	if( !json )
	{
		err = JRPC_JSON_ERR;
		return err;
	}
	err = _jrpc_send(client->fd, json);
	free(json);
	if( -1 == err )
	{
		return JRPC_CONNECTION_CLOSE;
	}
	err = _jrpc_send(client->fd, JRPC_DELIMITER);
	if( -1 ==  err )
	{
		return JRPC_CONNECTION_CLOSE;
	}


	jrpc_result_t * result = calloc(1, sizeof(jrpc_result_t));
	if( !result )
	{
		err = -1;
		return err;
	}
	err = _jrpc_get_response(client, callid, result);
	if( err < 0 )
	{
		free(result);
		return err;
	}
	if( result->err == 0 && result->type == STRING )
	{
		int slen = strlen(result->s);
		if( client->sessionid )
			free(client->sessionid);
		client->sessionid = malloc( slen + 1 );
		if( client->sessionid )
		{
			memset(client->sessionid, '\0', slen + 1);
			strncpy(client->sessionid, result->s, slen);
			printf("obtain sessionid:%s\n",client->sessionid);
		}
	}
	err = result->err;
	jrpc_free_result(result);

	return err;
}

int _jrpc_make_json(jrpc_client_t * client, unsigned int callid, const char * method,
		const char * params, char * json, int jsonlen)
{
	cJSON * root = cJSON_CreateObject();
	if( !root )
	{
		return -1;
	}

	cJSON_AddStringToObject(root, "jsonrpc", "2.0");
	cJSON_AddStringToObject(root, "method", method);
	if( callid != 0 )
	{
		cJSON_AddNumberToObject(root, "id", callid);
	}
	if( client->sessionid )
	{
		cJSON_AddStringToObject(root, "sid", client->sessionid);
	}
	cJSON * p = cJSON_Parse(params);
	if( p )
	{
		cJSON_AddItemToObject(root, "params", p);
	}
	char * jsonstring = cJSON_Print(root);
	memset(json, '\0', jsonlen);
	int retval = -1;
	if( strlen(jsonstring) < jsonlen )
	{
		strncpy(json, jsonstring, jsonlen);
		retval = 0;
	}
	cJSON_Delete(root);
	free(jsonstring);
	return retval;
}



int jrpc_make_params(const char * fmt, va_list ap, char * out_buff, int buff_size)
{
	if (!out_buff || buff_size < 1) {
		return -1;
	}

	memset(out_buff, '\0', buff_size);

	strcat(out_buff, "[");
	int len = strlen(out_buff);

	while( fmt && *fmt != '\0' && len < buff_size - 1)
	{
		if( *fmt == '%' )
		{
			switch( *(fmt+1) )
			{
			case	'd':
				{
					int k = va_arg(ap,int);
					sprintf(out_buff + len, "%d,", k);
				}break;
			case	'f':
				{
					double b = va_arg(ap, double);
					sprintf(out_buff + len, "%lf,", b);
				}break;
			case	's':
				{
					char * s = va_arg(ap, char*);
					sprintf(out_buff + len, "\"%s\",", s);
				}break;
			case	'u':
				{
					unsigned int ui = va_arg(ap, unsigned int);
					sprintf(out_buff + len, "%u,", ui);
				}break;
			case	'l':
				{
					switch( *(fmt+2) )
					{
						case	'd':
							{
								long l = va_arg(ap, long);
								sprintf(out_buff + len, "%ld,", l);
							}break;
						case	'u':
							{
								unsigned long lu = va_arg(ap, unsigned long);
								sprintf(out_buff + len, "%lu,", lu);
							}break;
						case	'f':
							{
								double dbl = va_arg(ap, double long);
								sprintf(out_buff + len, "%lf,", dbl);
							}break;
						case	'%':	break;
						default:break;
					}
					fmt++;
				}break;
			case	'%':
				fmt++;break;
			default:break;
			}
			len = strlen(out_buff);
		}
		fmt++;
	}
	char * p = strrchr(out_buff, ',');
	if( p )
		*p = ' ';
	strcat(out_buff, "]");
	return 0;
}



jrpc_result_t * jrpc_fmtcall(jrpc_client_t * client, const char * method, const char* fmt,...)
{
	char params[JRPC_MAX_PARAMS_SIZE];

	va_list ap;
	va_start(ap, fmt);
	jrpc_make_params(fmt, ap, params, sizeof params);
	va_end(ap);

	return jrpc_call(client, method, params);
}









#define JRPC_CLIENT_RETRY_NTIMES	1

jrpc_result_t * jrpc_call(jrpc_client_t * client, const char * method, const char * params)
{
	if( !client || !method )
	{
		return NULL;
	}
	int err = -1;

	err = _jrpc_reconnect(client);
	if( err < 0 )
	{
		printf("reconnect error\n");
		return NULL;
	}

	char json[JRPC_PACKAGE_SIZE - 10];
	unsigned int callid = _jrpc_gen_callid();
	if( -1 == _jrpc_make_json(client, callid, method, params, json, sizeof json) )

	{
		return NULL;
	}
	jrpc_result_t * result = NULL;
	int retry = JRPC_CLIENT_RETRY_NTIMES;
	do{
		err = _jrpc_reconnect(client);
		if( err < 0 )
		{
			return NULL;
		}
		err = _jrpc_send_data(client, json);
		if( err == JRPC_CONNECTION_CLOSE  )
		{
			if( retry-- )
				continue;
			else
				break;
		}
		result = calloc(1, sizeof(jrpc_result_t));
		if( !result )
		{
			break;
		}
		err = _jrpc_get_response(client, callid, result);

		_jrpc_close_connection(client);

	}while(err == JRPC_CONNECTION_CLOSE);

	if (result->err == JRPC_WAIT_TIMEOUT || result->err == JRPC_SELECT_ERR) {
		jrpc_free_result(result);
		result = NULL;
	}
	
	return result;
}

int jrpc_fmtnotify(jrpc_client_t * client, const char * method, const char* fmt, ...)
{
	char params[JRPC_MAX_PARAMS_SIZE];

	va_list ap;
	va_start(ap, fmt);
	jrpc_make_params(fmt, ap, params, sizeof params);
	va_end(ap);

	return jrpc_nofity(client, method, params);
}




int jrpc_nofity(jrpc_client_t * client, const char * method, const char * params)
{
	if( !client || !method )
	{
		return -1;
	}
	int err = -1;
	err = _jrpc_reconnect(client);
	if( err < 0 )
	{
		return -1;
	}
	char json[JRPC_PACKAGE_SIZE - 10];
	unsigned int callid = 0;
	if( -1 == _jrpc_make_json(client, callid, method, params, json, sizeof json) )

	{
		return -1;
	}
	int retry = JRPC_CLIENT_RETRY_NTIMES;
	do{
		err = _jrpc_reconnect(client);
		if( err < 0 )
		{
			return -1;
		}
		err = _jrpc_send_data(client, json);
		if( err == JRPC_CONNECTION_CLOSE )
		{
			if( retry-- )
				continue;
			else
				break;

		}
		_jrpc_close_connection(client);
	}while(err == JRPC_CONNECTION_CLOSE);

	return err;
}




int jrpc_client_register(jrpc_client_t * client, const char* funcdesc)
{
	return 0;
}

jrpc_client_t * jrpc_client_create(char * serverip, int serverport, int mode, int towait)
{
	jrpc_client_t * client = NULL;

	if( mode & JRPC_CLIMODE_LOCALSOCKET )
	{
		if( !(client = calloc(1, sizeof(jrpc_client_t))) )
		{
			return NULL;
		}
		memset(&client->srv_un_addr, '\0', sizeof client->srv_un_addr);
		client->srv_un_addr.sun_family = AF_LOCAL;
		unlink(JRPC_CLIENT_LOCALSOCK_PATH);
		strcpy(client->srv_un_addr.sun_path, JRPC_SERVER_LOCALSOCK_PATH);
	}
	else
	{
		if( !serverip || serverport < 1024 || serverport > 65535)
		{
			return NULL;
		}
		if( !(client = calloc(1, sizeof(jrpc_client_t))) )
		{
			return NULL;
		}
		memset(&client->srv_in_addr, '\0', sizeof client->srv_in_addr);
		client->srv_in_addr.sin_family = AF_INET;
		client->srv_in_addr.sin_port = htons(serverport);
		if( !inet_pton(AF_INET, serverip, &client->srv_in_addr.sin_addr) )
		{
			jrpc_client_destroy(client);
			return NULL;
		}
	}


	client->mode = mode;
	client->towait = towait;
	client->fd = -1;
	return client;
}

int jrpc_client_destroy(jrpc_client_t * client)
{
	if( client )
	{
		close(client->fd);
		shutdown(client->fd, SHUT_RDWR);
		free(client);
	}
	return 0;
}

/*

int main ()
{
	jrpc_client_t * client;
	client = jrpc_client_create("192.168.186.128", 12345, JRPC_CLIMODE_LOCALSOCKET, 10);
	jrpc_result_t * result;

	int i = 1000000;
	while( i-- )
	{


		result = jrpc_call(client, "rpc_echo", "[\"echo this echoecho this echoecho this echoecho this echoecho this echoecho this echo\"]");
		char * s = jrpc_get_result_string(result);
		printf("rpc_echo: %s\n", s);
		jrpc_free_result(result);


		retval = jrpc_nofity(client, "rpc_addd", "[5.3,323.1]");
		printf("addd2:%d\n", retval);



		result = jrpc_call(client, "rpc_echo", "[\"echo this echoecho this echoecho this echoecho this echoecho this echoecho this echo\"]");
		char * s = jrpc_get_result_string(result);
		printf("rpc_echo: %s\n", s);
		jrpc_free_result(result);

	result = jrpc_call(client, "rpc_addd", "[5.3, 323.1]");
	double b = jrpc_get_result_double(result);
	printf("addd1: %f\n", b);
	jrpc_free_result(result);


	int retval = jrpc_nofity(client, "rpc_addd", "[5.3,323.1]");
	printf("addd2:%d\n", retval);

	result = jrpc_call(client, "rpc_addd", "[5.3,323.1]");
	double d = jrpc_get_result_double(result);
	printf("addd3: %f\n", d);
	jrpc_free_result(result);

	}

	jrpc_client_destroy(client);
}
*/

