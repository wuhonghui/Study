/* digest auth handler */
/* some code copied from libmicrohttpd and lighttpd */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "httplib-internal.h"
#include "http_digest_auth.h"
#include "utils/md5.h"
#include "utils/timing.h"
#include "remote_config.h"
#if 0
static const char *str_http_unauthorized =
"<!DOCTYPE HTML>\
<html>\
<head>\
<title>401 Authorization Required</title>\
</head>\
<body>\
<h1>401 Authorization Required</h1>\
</body>\
</html>";

#define HTTP_REALM 	"TP-Link IP-Camera"
#define HTTP_OPAQUE "64943214654649846565646421"

#define HEX_MD5_SIZE 32
#define NONCE_MD5_SIZE (HEX_MD5_SIZE)
#define NONCE_SEC_SIZE 8
#define NONCE_NSEC_SIZE 8

#define NONCE_SIZE \
	((NONCE_MD5_SIZE) + (NONCE_SEC_SIZE) + (NONCE_NSEC_SIZE))

#define NONCE_ARRAY_SIZE 40
#define NONCE_TIMEOUT 30

struct digest_kv{
	const char *key;
	int key_len;
	char **ptr;
};

struct http_digest_nonce{
	char nonce[NONCE_SIZE + 1];
	int nc;
};

static char http_rnd[HEX_MD5_SIZE + 1];
static int is_param_inited = 0;
static struct http_digest_nonce nonce_array[NONCE_ARRAY_SIZE];

static uint32_t nonce_get_sec(char *nonce)
{
	if (!nonce)
		return -1;

	uint32_t sec = 0;
	uint8_t nonce_sec[NONCE_SEC_SIZE + 1];
	memcpy(nonce_sec, nonce + NONCE_MD5_SIZE, NONCE_SEC_SIZE);
	nonce_sec[NONCE_SEC_SIZE] = '\0';
	sec = strtoul((const char *)nonce_sec, (char **)NULL, 16);

	return sec;
}

static long nonce_get_nsec(char *nonce)
{
	if (!nonce)
		return -1;

	uint32_t nsec = 0;
	uint8_t nonce_nsec[NONCE_NSEC_SIZE + 1];
	memcpy(nonce_nsec, nonce + NONCE_MD5_SIZE + NONCE_SEC_SIZE, NONCE_NSEC_SIZE);
	nonce_nsec[NONCE_NSEC_SIZE] = '\0';
	nsec = strtoul((const char *)nonce_nsec, (char **)NULL, 16);

	return (long)nsec;
}


static int check_nonce_nc(char *nonce, int nc)
{
	int i = 0;
	int len = strlen(nonce);
	int nonce_sec = nonce_get_sec(nonce);
	int nonce_nsec = nonce_get_nsec(nonce);

	int min_nonce_sec = INT_MAX;
	int min_nonce_index = 0;
	dbprintf("check nonce: %s, nc: %d\n", nonce, nc);

	/* check if nonce already exist */
	for (i = 0; i < NONCE_ARRAY_SIZE; ++i) {
		if (strcmp(nonce, nonce_array[i].nonce) == 0) {
			if (nc <= nonce_array[i].nc) {
				dbprintf("playback attack\n");
				return -1;
			} else {
				dbprintf("update nonce count\n");
				nonce_array[i].nc = nc;
				return 0;
			}
		}
	}

	/* if nonce not found and nc == 1, store nonce in the array */
	if (i == NONCE_ARRAY_SIZE && nc == 1) {
		for (i = 0; i < NONCE_ARRAY_SIZE; ++i) {
			if (nonce_get_sec(nonce_array[i].nonce) < min_nonce_sec) {
				min_nonce_index = i;
				min_nonce_sec = nonce_get_sec(nonce_array[i].nonce);
			}else if (nonce_get_sec(nonce_array[i].nonce) == min_nonce_sec &&
				min_nonce_sec != 0) {
				if (nonce_get_nsec(nonce_array[i].nonce) <
					nonce_get_nsec(nonce_array[min_nonce_index].nonce)) {
					min_nonce_index = i;
				}
			}
		}
		dbprintf("new nonce store in %d\n", min_nonce_index);
		strcpy(nonce_array[min_nonce_index].nonce, nonce);
		nonce_array[min_nonce_index].nc = nc;
		return 0;
	} else {
		return -1;
	}

	return -1;
}


static void calculate_nonce (uint32_t nonce_sec,
		 long nonce_nsec,
		 const char *method,
		 const char *rnd,
		 unsigned int rnd_size,
		 const char *uri,
		 const char *realm,
		 char *nonce);

static void digest_calc_ha1 (const char *alg,
		 const char *username,
		 const char *realm,
		 const char *password,
		 const char *nonce,
		 const char *cnonce,
		 char *sessionkey);

static void digest_calc_response (const char *ha1,
		      const char *nonce,
		      const char *noncecount,
		      const char *cnonce,
		      const char *qop,
		      const char *method,
		      const char *uri,
		      const char *hentity,
		      char *response);


int http_digest_auth_check(struct http_connection *hc)
{
	char *username = NULL;
	char *realm = NULL;
	char *nonce = NULL;
	char *uri = NULL;
	char *algorithm = NULL;
	char *qop = NULL;
	char *cnonce = NULL;
	char *nc = NULL;
	char *respons = NULL;

	char *authorization = NULL;
	int auth_ok = 0;
	int is_nonce_timeout = 0;

	int i = 0;

	if (!is_param_inited) {
		memset(&nonce_array, 0, sizeof(nonce_array));
		srandom(time(NULL));
		for (i = 0; i < sizeof(http_rnd) - 1; i++) {
			http_rnd[i] = (random()%10) + '0';
		}
		http_rnd[i] = '\0';
		is_param_inited++;
	}

	if (!hc)
		return -1;

	authorization = http_get_request_header(hc, "Authorization");

	if (authorization) {

#define S(x) \
	x, sizeof(x)-1, NULL
		struct digest_kv dkv[10] = {
			{ S("username=") },
			{ S("realm=") },
			{ S("nonce=") },
			{ S("uri=") },
			{ S("algorithm=") },
			{ S("qop=") },
			{ S("cnonce=") },
			{ S("nc=") },
			{ S("response=") },
			{ NULL, 0, NULL }
		};
#undef S

		dkv[0].ptr = &username;
		dkv[1].ptr = &realm;
		dkv[2].ptr = &nonce;
		dkv[3].ptr = &uri;
		dkv[4].ptr = &algorithm;
		dkv[5].ptr = &qop;
		dkv[6].ptr = &cnonce;
		dkv[7].ptr = &nc;
		dkv[8].ptr = &respons;

		if (0 == strncmp(authorization, "Digest ", 7)) {
			authorization += 7;

			char *c = authorization;
			char *e = NULL;
			for (; *c != '\0'; c++) {
				while (*c == ' ' || *c == '\t')
					c++;
				if (*c == '\0')
					break;

				for (i = 0; dkv[i].key != NULL; i++) {
					if ((0 == strncmp(c, dkv[i].key, dkv[i].key_len))) {
						if ((c[dkv[i].key_len] == '"') &&
						    (NULL != (e = strchr(c + dkv[i].key_len + 1, '"')))) {
							/* value with "..." */
							*(dkv[i].ptr) = c + dkv[i].key_len + 1;
							c = e;

							*e = '\0';
						} else if (NULL != (e = strchr(c + dkv[i].key_len, ','))) {
							/* value without "...", terminated by ',' */
							*(dkv[i].ptr) = c + dkv[i].key_len;
							c = e;

							*e = '\0';
						} else {
							/* value without "...", terminated by EOL */
							*(dkv[i].ptr) = c + dkv[i].key_len;
							c += strlen(c) - 1;
						}
					}
				}
			}

			//for (i = 0; i < sizeof(dkv)/sizeof(struct digest_kv) - 1; ++i)
				//dbprintf("%s : %s\n", dkv[i].key, *(dkv[i].ptr));

			if (!username || !realm || !nonce || !uri ||
				(qop && (!nc || !cnonce)) || !respons ) {
				dbprintf("digest field missing\n");
				goto out;
			}

			if (strncmp(uri, hc->uri, strlen(hc->uri))) {
				goto out;
			}

			/* extract nonce time from nonce */
			int len = strlen(nonce);
			int nonce_sec = nonce_get_sec(nonce);
			long nonce_nsec = nonce_get_nsec(nonce);

			char *end = NULL;
			int nc_count = strtoul (nc, &end, 16);
			if (*end != '\0' || nc_count > LONG_MAX) {
				goto out;
			}

			char connection_nonce[NONCE_SIZE + 1];
			if(nc_count == 1){
				calculate_nonce(nonce_sec, nonce_nsec, hc->method, http_rnd, strlen(http_rnd),
					hc->uri, HTTP_REALM, connection_nonce);
			} else {
				uint32_t t_now = (uint32_t) monotonic_time();
				if (nonce_sec + NONCE_TIMEOUT < t_now ||
					nonce_sec + NONCE_TIMEOUT < nonce_sec) {
					is_nonce_timeout = 1;
					goto out;
				} else {
					strncpy(connection_nonce, nonce, sizeof(connection_nonce));
					connection_nonce[NONCE_SIZE] = '\0';
				}
			}

			if (strcmp(nonce, connection_nonce)) {
				goto out;
			}

			if (nc && cnonce && respons && qop &&
				(strcmp(qop, "auth") == 0 || strcmp(qop, "") == 0)) {
				if (check_nonce_nc(nonce, nc_count)) {
					goto out;
				}

				char ha1[33];
				char respexp[33];
				char password[33];
				memset(password, 0, sizeof(password));
				if (config_get_username_password(username, password)) {
					goto out;
				}
				digest_calc_ha1("md5", username, HTTP_REALM, password,
					nonce, cnonce, ha1);
				digest_calc_response(ha1, nonce, nc,
					cnonce, qop, hc->method, uri, NULL, respexp);
				if (strcmp(respexp, respons) == 0) {
					hc->username = strdup(username);
					auth_ok = 1;
				}
			}
		}else if (0 == strncmp(authorization, "Basic ", 6)) {
			if (strlen(authorization) < 256) {
				char decoded_buffer[256];
				char password[33];
				char *basic_username = NULL;
				char *basic_password = NULL;
				if (0 == Base64Decode(authorization+6, decoded_buffer, sizeof(decoded_buffer))) {
					basic_username = basic_password = decoded_buffer;
					while (*basic_password != '\0' && *basic_password != ':')
						basic_password++;
					if (*basic_password == ':') {
						*basic_password++ = '\0';

						memset(password, 0, sizeof(password));
						if (config_get_username_password(basic_username, password)) {
							goto out;
						}

						if (strcmp(password, basic_password) == 0) {
							hc->username = strdup(basic_username);
							auth_ok = 1;
						}
					}
				}
			}
		}
	}

out:
	if (!auth_ok) {
		char newnonce[NONCE_SIZE + 1];

		calculate_nonce((uint32_t)monotonic_time(),
			monotonic_time_nsec(),
			hc->method, http_rnd, strlen(http_rnd),
			hc->uri, HTTP_REALM, newnonce);

		http_header_start(hc, 401, "Unauthorized", NULL);
		http_header_add(hc, "Pragma: no-cache");
    	http_header_add(hc, "Cache-Control: no-cache");
		http_header_clength(hc, 0);
		evbuffer_add_printf(hc->evbout,
			"WWW-Authenticate: Digest realm=\"%s\",qop=\"auth\",nonce=\"%s\",opaque=\"%s\"%s\r\n",
			HTTP_REALM, newnonce, HTTP_OPAQUE,
			is_nonce_timeout? ",stale=\"true\"" : "");
		http_header_end(hc);

		http_evbuffer_writeout(hc, 0);

		http_connection_free(hc);

		return -1;
	}

	return 0;
}

static const char hex_chars[] = "0123456789abcdef";
static void cvthex(const unsigned char *bin, int len, char *hex)
{
	int i;

	for (i = 0; i < len; ++i) {
    	hex[i * 2] = hex_chars[(bin[i] >> 4) & 0x0f];
    	hex[i * 2 + 1] = hex_chars[(bin[i] & 0x0f)];
	}
	hex[len * 2] = '\0';
}

static void calculate_nonce (uint32_t nonce_sec,
		 long nonce_nsec,
		 const char *method,
		 const char *rnd,
		 unsigned int rnd_size,
		 const char *uri,
		 const char *realm,
		 char *nonce)
{
	struct MD5Context md5;
	unsigned char timestamp[8];
	unsigned char tmpnonce[MD5_DIGEST_SIZE];
	char timestamphex[sizeof(timestamp) * 2 + 1];

	MD5Init (&md5);
	timestamp[0] = (nonce_sec & 0xff000000) >> 0x18;
	timestamp[1] = (nonce_sec & 0x00ff0000) >> 0x10;
	timestamp[2] = (nonce_sec & 0x0000ff00) >> 0x08;
	timestamp[3] = (nonce_sec & 0x000000ff);
	timestamp[4] = (nonce_nsec & 0xff000000) >> 0x18;
	timestamp[5] = (nonce_nsec & 0x00ff0000) >> 0x10;
	timestamp[6] = (nonce_nsec & 0x0000ff00) >> 0x08;
	timestamp[7] = (nonce_nsec & 0x000000ff);
	MD5Update (&md5, timestamp, 8);
	MD5Update (&md5, ":", 1);
	MD5Update (&md5, method, strlen(method));
	MD5Update (&md5, ":", 1);
	if (rnd_size > 0)
	MD5Update (&md5, rnd, rnd_size);
	MD5Update (&md5, ":", 1);
	MD5Update (&md5, uri, strlen(uri));
	MD5Update (&md5, ":", 1);
	MD5Update (&md5, realm, strlen(realm));
	MD5Final (tmpnonce, &md5);
	cvthex (tmpnonce, sizeof (tmpnonce), nonce);
	cvthex (timestamp, 8, timestamphex);
	strncat (nonce, timestamphex, 16);
}

static void digest_calc_ha1 (const char *alg,
		 const char *username,
		 const char *realm,
		 const char *password,
		 const char *nonce,
		 const char *cnonce,
		 char *sessionkey)
{
	struct MD5Context md5;
	unsigned char ha1[MD5_DIGEST_SIZE];

	MD5Init (&md5);
	MD5Update (&md5, username, strlen (username));
	MD5Update (&md5, ":", 1);
	MD5Update (&md5, realm, strlen (realm));
	MD5Update (&md5, ":", 1);
	MD5Update (&md5, password, strlen (password));
	MD5Final (ha1, &md5);
	if (0 == strcasecmp (alg, "md5-sess"))
	{
		MD5Init (&md5);
		MD5Update (&md5, ha1, sizeof (ha1));
		MD5Update (&md5, ":", 1);
		MD5Update (&md5, nonce, strlen (nonce));
		MD5Update (&md5, ":", 1);
		MD5Update (&md5, cnonce, strlen (cnonce));
		MD5Final (ha1, &md5);
	}
	cvthex (ha1, sizeof (ha1), sessionkey);
}

static void digest_calc_response (const char *ha1,
		      const char *nonce,
		      const char *noncecount,
		      const char *cnonce,
		      const char *qop,
		      const char *method,
		      const char *uri,
		      const char *hentity,
		      char *response)
{
  struct MD5Context md5;
  unsigned char ha2[MD5_DIGEST_SIZE];
  unsigned char resphash[MD5_DIGEST_SIZE];
  char ha2hex[HASH_MD5_HEX_LEN + 1];

  MD5Init (&md5);
  MD5Update (&md5, method, strlen(method));
  MD5Update (&md5, ":", 1);
  MD5Update (&md5, uri, strlen(uri));
  MD5Final (ha2, &md5);
  cvthex (ha2, MD5_DIGEST_SIZE, ha2hex);
  MD5Init (&md5);
  /* calculate response */
  MD5Update (&md5, ha1, HASH_MD5_HEX_LEN);
  MD5Update (&md5, ":", 1);
  MD5Update (&md5, nonce, strlen(nonce));
  MD5Update (&md5, ":", 1);
  if ('\0' != *qop)
    {
      MD5Update (&md5, noncecount, strlen(noncecount));
      MD5Update (&md5, ":", 1);
      MD5Update (&md5, cnonce, strlen(cnonce));
      MD5Update (&md5, ":", 1);
      MD5Update (&md5, qop, strlen(qop));
      MD5Update (&md5, ":", 1);
    }
  MD5Update (&md5, ha2hex, HASH_MD5_HEX_LEN);
  MD5Final (resphash, &md5);
  cvthex (resphash, sizeof (resphash), response);
}
#endif
