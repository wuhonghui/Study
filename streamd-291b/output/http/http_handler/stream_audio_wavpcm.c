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

#include "input_sound_device.h"

#include "utils/wav.h"
#include "utils/timing.h"

#include "media_source.h"
#include "media_receiver.h"

static void free_media_receiver_priv(void *priv)
{
	struct media_receiver *receiver = (struct media_receiver *)priv;
	media_source_del_receiver(receiver);
	media_receiver_free(receiver);
}

static int http_audio_wavpcm_send_fragment(struct frame_buf *buf, void *arg)
{
	struct http_connection *hc = arg;
	int ret = 0;

	if (!buf) {
		http_connection_free(hc);
		return -1;
	}

	/* allow to buffer not more than 2 seconds */
	if (evbuffer_get_length(hc->evbout) < 8000) {
        /* copy video frame to evbuffer */
        evbuffer_add(hc->evbout, buf->data, buf->size);
    }

    /* write out evbuffer, disallow skip */
    ret = http_evbuffer_writeout(hc, 0);
    if (ret < 0) {
        http_connection_free(hc);
		return -1;
    }

	return 0;
}


void http_stream_audio_wavpcm_cb(evutil_socket_t fd, short events, void *arg)
{
    int ret = 0;
    struct http_connection *hc = (struct http_connection *)arg;
	struct media_source *source = NULL;
	struct media_receiver *receiver = NULL;

	source = server_media_source_find("LIVE-SOUND-PCM");
	if (!source) {
		http_connection_free(hc);
		return;
	}
	
	receiver = media_receiver_new();
	if (!receiver) {
		http_connection_free(hc);
		return;
	}
	media_receiver_set_callback(receiver, http_audio_wavpcm_send_fragment, hc);
	
	hc->priv = receiver;
	hc->free_priv = free_media_receiver_priv;
	
	media_source_add_receiver(source, receiver);

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
