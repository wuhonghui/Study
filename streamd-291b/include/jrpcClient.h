#ifndef __JRPCCLIENT_H__
#define __JRPCCLIENT_H__
#include <stdarg.h>

typedef struct _jrpc_result_t jrpc_result_t;

void jrpc_free_result(jrpc_result_t * result);

int jrpc_get_result_int(const jrpc_result_t * result, int *address);

int jrpc_get_result_double(const jrpc_result_t * result, double *address);

const char * jrpc_get_result_string(const jrpc_result_t * result);




typedef struct _jrpc_client_t jrpc_client_t;


#define JRPC_MAX_PARAMS_SIZE	1024	/* # of max size of a json param array */


/*	client mode	*/
#define JRPC_CLIMODE_LOCALSOCKET	0x1
#define	JRPC_CLIMODE_SESSION		0x2
#define JRPC_CLIMODE_PERSIST		0x4
/*
 *	@serverip:	ipv4 in dot format.example: 192.168.1.1
 *	@serverport: 1024 < @serverport < 65535
 *	@mode:		describe above, use a '|' to multiinclude them
 				NOTE: when JRPC_CLIMODE_LOCALSOCKET is specified,
 				arguments serverip && serverport are ignored

 *	@towait:	time to wait for rpc call timeout(in seconds)
 */
jrpc_client_t * jrpc_client_create(char* serverip, int serverport, int mode, int towait);

int jrpc_client_destroy(jrpc_client_t * client);

int jrpc_client_register(jrpc_client_t * client, const char* funcdesc);


/*
 *	The followings are rpc calls u may use. if u are interested in the return value of
 *	your calling method, use function jrpc_client_[j]wcall. otherwise, instead.
 *
 *	Remember: call jrpc_free_result() to free jrpc_result_t * after calling these functions.
 *
 */


/*
 *	@params: parameters of the @method. format:[param1,param2,...]
 *	example: char * params = "[ 1, 2.3, \"abcde\"]";
 */
jrpc_result_t * jrpc_call(jrpc_client_t * client, const char * method, const char * params);

/**
 *	@fmt: format of the rest params . [ %d -> int  %f -> float/double %s -> char* ]
 *
 *	example: jrpc_fmtcall(client, "rpc_addd", "%f dkd %%f d %f", 5.3, 323.1); " dkd %%f d " will be ignored.
 */
jrpc_result_t * jrpc_fmtcall(jrpc_client_t * client, const char * method, const char* fmt,...);




/* return 0 if rpc call successfully sent */
int jrpc_nofity(jrpc_client_t * client, const char * method, const char * params);


int jrpc_fmtnotify(jrpc_client_t * client, const char * method, const char* fmt, ...);

int jrpc_make_params(const char * fmt, va_list ap, char * out_buff, int buff_size);


#endif
