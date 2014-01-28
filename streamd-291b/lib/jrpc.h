#ifndef __JRPC_H__
#define __JRPC_H__


/*
 *
 * http://www.jsonrpc.org/specification
 *
 * code	message	meaning
 * -32700	Parse error	Invalid JSON was received by the server.
 * An error occurred on the server while parsing the JSON text.
 * -32600	Invalid Request	The JSON sent is not a valid Request object.
 * -32601	Method not found	The method does not exist / is not available.
 * -32602	Invalid params	Invalid method parameter(s).
 * -32603	Internal error	Internal JSON-RPC error.
 * -32000 to -32099	Server error	Reserved for implementation-defined server-errors.
 */
#define JRPC_PARSE_ERROR		-32700
#define JRPC_INVALID_REQUEST	-32600
#define JRPC_METHOD_NOT_FOUND	-32601
#define JRPC_INVALID_PARAMS		-32602
#define JRPC_INTERNAL_ERROR		-32603

#define JRPC_SESSION_TIMEOUT	-32000
#define JRPC_SESSION_REQUIRED	-32001

#define JRPC_DELIMITER 			"\n\n"
#define JRPC_PACKAGE_SIZE		1024	/* # of max size of a json string with delimiter */
#define JRPC_MAX_PARAMETER		12		/* # of max params of rpc method */
#define JRPC_CLIENT_LOCALSOCK_PATH	"/tmp/jrpcC.socket"
#define JRPC_SERVER_LOCALSOCK_PATH	"/tmp/jrpcS.socket"


/* the return is const string, need no free */
const char* jrpc_get_error_msg(int error);


#endif /* __CMRPCSERVER_H__ */
