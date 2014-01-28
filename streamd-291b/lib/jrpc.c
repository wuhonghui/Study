#include"jrpc.h"

typedef struct _error_msg_t{
	int 	err;
	char*	msg;
}error_msg_t;



error_msg_t jrpc_error_msg[] = {
	{JRPC_PARSE_ERROR,		"Parse error"},
	{JRPC_INVALID_REQUEST,	"Invalid Request"},
	{JRPC_METHOD_NOT_FOUND,	"Method not found"},
	{JRPC_INVALID_PARAMS,	"Invalid params"},
	{JRPC_INTERNAL_ERROR,	"Internal error"},

	{JRPC_SESSION_TIMEOUT,	"Session timeout"},
	{JRPC_SESSION_REQUIRED,	"Session required"},

/*
	{JRPC_PARAMETER_NULL,	"Use a Null Parameter"},
	{JRPC_METHOD_UNREGISTED,"Call a Unregistered Method"},
	{JRPC_JSON_CREATE_ERR,	"cJSON Create Failed"},
	{JRPC_OUT_OF_PACKAGE,	"Sizeof Json Data Out Of Package Length"},
	{JRPC_SOCK_INIT_ERR,	"Create Socket Error"},
	{JRPC_SOCK_CONNECT_ERR,	"Socket Connect Error"},
	{JRPC_WAIT_TIMEOUT,		"Client Wait Response TimeOut"},
	{JRPC_SELECT_ERR,		"Client Select Error"},
	{JRPC_RECEIVE_NOTHING,	"Client Receive Nothing"},
	{JRPC_RESPONSE_FORMAT_ERR,	"Response not a json format"},
	{JRPC_PROTOCAL_UNMATCH,	"Protocal Not Match"},
	{JRPC_RECV_ERR,			"Receive Data Error"},
	{JRPC_CALL_ERR,			"Method Call Return Error"},
	{JRPC_CONNECTION_CLOSE, "Unexpected Connection Close"},
*/
	
	{0,						"Unkown error"} /* this is the default error message */
};

const char* jrpc_get_error_msg(int error)
{
	int i = 0;
	while ( jrpc_error_msg[i].err && 
			jrpc_error_msg[i].err != error ) {
		i++;		
	}
	return jrpc_error_msg[i].msg;
}

