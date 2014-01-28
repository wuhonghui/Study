#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include "common.h"
#include "input_video.h"
#include "jrpcClient.h"
#include "remote_config.h"

struct video_stream_config {
	int fps;
};
struct video_config {
	int width;
	int height;
};

static int http_port = 8080;
static struct video_config vdconfig= {
	.width = 640,
	.height = 480,
};
static struct video_stream_config vsconfig = {
	.fps = 20,
};

static int use_jrpc = 1;

jrpc_result_t * jrpc_begin(char *method, char *params, ...)
{
	if (!method || !params)	{
		dbprintf("failed to begin jrpc: null pointer\n");
		return NULL;
	}
	char paramstr[1000];
	va_list ap;
	jrpc_client_t * client;
	jrpc_result_t * result;

	va_start(ap, params);
	vsnprintf(paramstr, 1000, params, ap);
	va_end(ap);

	client = jrpc_client_create("127.0.0.1", 12345, JRPC_CLIMODE_LOCALSOCKET, 3);
	if (!client)
		return NULL;

	result = jrpc_call(client, method, paramstr);

	/* release the rpc client */
	jrpc_client_destroy(client);
	
	return result;
}

void jrpc_end(jrpc_result_t *result)
{
	if (!result) {
		dbprintf("failed to begin jrpc: null pointer\n");
		return;
	}
	jrpc_free_result(result);
}


int config_jrpc_load()
{
	jrpc_result_t *result = NULL;
	int gotfailed = 0;

	result = jrpc_begin("swStreamServerGetVideoWidth", "[]");
	if (result) {
		if (jrpc_get_result_int(result, &(vdconfig.width)) == 0){
			dbprintf("Get vdconfig.width %d from control server\n", vdconfig.width);
		}
		jrpc_end(result);
	} else {
		return -1;
	}

	result = jrpc_begin("swStreamServerGetVideoHeight", "[]");
	if (result) {
		if (jrpc_get_result_int(result, &(vdconfig.height)) == 0){
			dbprintf("Get vdconfig.height %d from control server\n", vdconfig.height);
		}
		jrpc_end(result);
	} else {
		return -1;
	}

	result = jrpc_begin("swStreamServerGetHttpPort", "[]");
	if (result) {
		if (jrpc_get_result_int(result, &(http_port)) == 0){
			dbprintf("Get http_port %d from control server\n", http_port);
		}
		jrpc_end(result);
	} else {
		return -1;
	}

	result = jrpc_begin("swStreamServerGetFps", "[]");
	if (result) {
		if (jrpc_get_result_int(result, &(vsconfig.fps)) == 0){
			dbprintf("Get fps %d from control server\n", vsconfig.fps);
		}
		jrpc_end(result);
	} else {
		return -1;
	}

	return 0;
}

static int isdigitstring(const char *str)
{
	if (!str || str[0] == '\0')
		return 0;

	int i = 0;

	while(str[i] != '\0') {
		if (!isdigit(str[i++]))
			return 0;
	}

	return 1;
}

static int do_load_section(const char *section)
{
    return 0;
}

static int do_load_param(const char *name, const char *val)
{
    if (!strcasecmp(name, "VideoCtrl_Stream_Server_HTTP_Port")) {
		if (isdigitstring(val)) {
        	http_port = atoi(val);
		}
    }
    if (!strcasecmp(name, "VideoCtrl_Stream_Server_Fps")) {
		if (isdigitstring(val)) {
        	vsconfig.fps = atoi(val);
		}
    }
    if (!strcasecmp(name, "VideoCtrl_Resolution_Width")) {
		if (isdigitstring(val)) {
        	vdconfig.width = atoi(val);
		}
    }
    if (!strcasecmp(name, "VideoCtrl_Resolution_Height")) {
		if (isdigitstring(val)) {
        	vdconfig.height = atoi(val);
		}
    }

    return 0;
}

#define CONFIG_FILE "/usr/local/config/ipcamera/videoctrl.conf"
#define USER_CONF_FILE "/usr/local/config/ipcamera/user.conf"
#include "param.h"

static unsigned char Base64Index[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

int Base64Decode(unsigned char *pIn, unsigned char *pOut, int nBufLen)
{
	unsigned char cByte1 = 0;  /*the first char*/
	unsigned char cByte2 = 0;
	unsigned char cByte3 = 0;
	unsigned char cByte4 = 0;
	int nInputLen = (NULL == pIn)?0:strlen(pIn);

	if(NULL == pIn 
		|| NULL == pOut
		|| nInputLen%4)
	{
		return -1;
	}

	while(nInputLen > 0)
	{		
		cByte1 = (unsigned char)((unsigned char*)strchr(Base64Index, *(pIn++)) - (unsigned char*)Base64Index);
		cByte2 = (unsigned char)((unsigned char*)strchr(Base64Index, *(pIn++)) - (unsigned char*)Base64Index);
		cByte3 = (unsigned char)((unsigned char*)strchr(Base64Index, *(pIn++)) - (unsigned char*)Base64Index);
		cByte4 = (unsigned char)((unsigned char*)strchr(Base64Index, *(pIn++)) - (unsigned char*)Base64Index);
	
		if(cByte1 > 64 || cByte2 > 64 || cByte3 > 64 || cByte4 > 64)
		{
			return -1;
		}

		if(nBufLen < 1) /*no space , just return*/
		{
			return -1;
		}
		*(pOut++) = ( ( cByte1 << 2 ) & 0xFC ) | ( ( cByte2 >> 4 ) & 0x03 );
		nBufLen--;
		if(64 == cByte3)
		{
			break;
		}

		if(nBufLen < 1)
		{
			return -1;
		}
		*(pOut++) = ( ( cByte2 << 4 ) & 0xF0 ) | ( ( cByte3 >> 2 ) & 0x0F );
		nBufLen--;
		if(64 == cByte4)
		{
			break;
		}

		if(nBufLen < 1)
		{
			return -1;
		}
		*(pOut++) = ( ( cByte3 << 6 ) & 0xC0 ) | ( ( cByte4 ) & 0x3F );
		nBufLen--;
		
		nInputLen -= 4;
	}

	*pOut = 0;
	return 0;
}

int config_file_load()
{
	load_param(do_load_section, do_load_param, CONFIG_FILE);
	return 0;
}

int config_load()
{
	if (config_jrpc_load()) {
		use_jrpc = 0;
		config_file_load();
	}
	
	return 0;
}

int config_get_http_port()
{
	return http_port;
}

int config_get_video_width()
{
	return vdconfig.width;
}

int config_get_video_height()
{
	return vdconfig.height;
}

int config_get_fps()
{
	return vsconfig.fps;
}

/* ugly..., simply make it work. */
static char *wanted_check_username = NULL;
static char *buffer_store_password = NULL;
static int found_user = 0;
static int found_password = 0;
static int do_load_user_section(const char *section)
{
    return 0;
}

static int do_load_user_param(const char *name, const char *val)
{
	if (!found_user && !found_password) {
		if (!strcasecmp(name, "name")) {
			if (!strcmp(wanted_check_username, val)) {
				found_user = 1;
			}
		}
	}

	if (found_user && !found_password) {
		if (!strcasecmp(name, "pass")) {
			// trans base64 to normal
			found_password = 1;
			Base64Decode((char *)val, buffer_store_password, 33);
		}
	}

	return 0;
}

int load_user_from_file(char *username, char *password)
{
	/* come on, start check it */
	wanted_check_username = username;
	buffer_store_password = password;
	found_user = 0;
	found_password = 0;
	
	int ret = 0;
	ret = load_param(do_load_user_section, do_load_user_param, USER_CONF_FILE);
	if (ret) {
		printf("Failed to load user info in %s\n", USER_CONF_FILE);
		use_jrpc = 1;
		return -1;
	}

	wanted_check_username = NULL;
	buffer_store_password = NULL;

	if (!found_user || !found_password) {
		return -1;
	}
	
	return 0;
}

int config_get_username_password(char *username, char *password)
{
	int ret = 0;
	int failed = 0;
	
	if (use_jrpc) {
		jrpc_result_t *result = NULL;

		result = jrpc_begin("swStreamServerGetUsernamePassword", "[\"%s\"]", username);
		if (result) {
			if (jrpc_get_result_string(result)) {
				strcpy(password, jrpc_get_result_string(result));
			} else {
				failed = 1;
			}
			jrpc_end(result);
		} else {
			use_jrpc = 0;
			failed = 1;
			printf("jrpc call failed, from now then on we use config file");
		}
	}

	// jrpc ok but not username found
	if (use_jrpc && failed) {
		return -1;
	} else if (use_jrpc && failed == 0) { // get username password ok
		return 0;
	}
	
	// jrpc call failed, check user conf file
	return load_user_from_file(username, password);
}

