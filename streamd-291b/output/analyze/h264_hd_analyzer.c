#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include "media_source.h"
#include "media_receiver.h"
#include "slog.h"

#include "h264_hd_analyzer.h"

static struct media_receiver *receiver = NULL;

static int h264_analyze_nalu(struct frame_buf *buf, void *arg)
{
	
	return 0;
}

int h264_hd_analyzer_init()
{
	struct media_source *source = NULL;
	
	source = server_media_source_find("LIVE-H264-HD");
	if (!source) {
		goto failed;
	}
	
	receiver = media_receiver_new();
	if (!receiver) {
		goto failed;
	}

	media_receiver_set_callback(receiver, h264_analyze_nalu, NULL);
	media_source_add_receiver(source, receiver);

	SLOG(SLOG_INFO, "H264 HD Analyzer Init Ok");
	
	return 0;
failed:
	h264_hd_analyzer_destroy();
	return -1;
}

void h264_hd_analyzer_destroy()
{
	if (receiver) {
		media_source_del_receiver(receiver);
		media_receiver_free(receiver);
		receiver = NULL;
	}
}
