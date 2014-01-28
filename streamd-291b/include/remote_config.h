#ifndef remote_config_H
#define remote_config_H

#ifdef __cplusplus
extern "C" {
#endif

#include "jrpcClient.h"

int config_get_fps();

int config_get_video_width();
int config_get_video_height();

int config_get_http_port();

int config_get_username_password(char *username, char *password);

jrpc_result_t * jrpc_begin(char *method, char *params, ...);
void jrpc_end(jrpc_result_t *result);

int	config_jrpc_load();
int config_file_load();

int config_load();

int Base64Decode(unsigned char *pIn, unsigned char *pOut, int nBufLen);

#ifdef __cplusplus
}
#endif

#endif
