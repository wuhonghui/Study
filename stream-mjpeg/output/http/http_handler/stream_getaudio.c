#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "httplib-internal.h"
#include "http-handler.h"
#include "http_digest_auth.h"

#include "input_sound.h"
#include "utils/wav.h"
#include "utils/timing.h"

static void free_audio_stream_priv(void *priv)
{
	sound_remove_receiver((struct generic_receiver *)priv);
}

static int http_audio_send_fragment(void *data, size_t len, void *arg)
{
	struct http_connection *hc = arg;
	int ret = 0;

	if (!data) {
		http_connection_free(hc);
		return -1;
	}

	/* allow to buffer not more than 2 seconds */
	if (evbuffer_get_length(hc->evbout) < 16000) {
        /* copy video frame to evbuffer */
        evbuffer_add(hc->evbout, data, len);
    }

    /* write out evbuffer, disallow skip */
    ret = http_evbuffer_writeout(hc, 0);
    if (ret < 0) {
        http_connection_free(hc);
		return -1;
    }

	return 0;
}


void http_stream_getaudio_cb(evutil_socket_t fd, short events, void *arg)
{
    int ret = 0;
    struct http_connection *hc = (struct http_connection *)arg;

	/* do auth */
	if (0 != http_digest_auth_check(hc)) {
		return;
	}

	/* if have username(from remote) */
	if (hc->username) {
		char identity[128];

		/* video:protocol:ip:port:username:begin_time */
		snprintf(identity, sizeof(identity), "audio:http:%s:%d:%s:%d",
			inet_ntoa(hc->sin.sin_addr),
			ntohs(hc->sin.sin_port),
			hc->username? hc->username : "",
			(int)monotonic_time());
		hc->priv = sound_add_receiver(identity, http_audio_send_fragment, hc);
	}else {
		hc->priv = sound_add_receiver(NULL, http_audio_send_fragment, hc);
	}

	if (!hc->priv) {
		if (sound_device_status() == DEVICE_RUN_STREAMING) {
			http_header_start(hc, 503, "Service Unavailable", "text/html");
		} else {
			http_header_start(hc, 500, "Internal Server Error", "text/html");
		}
		http_header_add(hc, "Pragma: no-cache");
	    http_header_add(hc, "Cache-Control: no-cache");
	    http_header_clength(hc, 0);
	    http_header_end(hc);

    	http_evbuffer_writeout(hc, 0);
		http_connection_free(hc);
		return;
	} else {
		hc->free_priv = free_audio_stream_priv;
	}

	/* build wav header */
	struct wav_pcm_header fileheader;
	int nBitsPerSample = 8;
	int samplerate = 8000;
	int channels = 1;

	fileheader.riffChkID = FOURCC_RIFF;
	fileheader.riffChkSize = 0x6DDD032;
	fileheader.waveChkID = FOURCC_WAVE;
	fileheader.fmtChkID = FOURCC_FMT;
	fileheader.fmtChkSize = 16;
	fileheader.wFormatTag = WAVE_FORMAT_PCM;
	fileheader.nChannels = channels;
	fileheader.nSamplesPerSec = samplerate;
	fileheader.nAvgBytesPerSec = (samplerate * channels * ((nBitsPerSample + 7) / 8));
	fileheader.nBlockAlign = channels * ((nBitsPerSample + 7) / 8);
	fileheader.wBitsPerSample = 8;
	fileheader.dataChkID = FOURCC_DATA;
	fileheader.dataChkSize = 0x6DDD000;

	/* set http header */
	http_header_start(hc, 200, "OK", "audio/x-wav");
	http_header_add(hc, "Pragma: no-cache");
	http_header_add(hc, "Cache-Control: no-cache");
	http_header_clength(hc, 0x6DDD000);
    http_header_end(hc);

	evbuffer_add(hc->evbout, &fileheader, sizeof(struct wav_pcm_header));

    ret = http_evbuffer_writeout(hc, 0);
    if (ret < 0) {
        http_connection_free(hc);
        return;
    }
}
