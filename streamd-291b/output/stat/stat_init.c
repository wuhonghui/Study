#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include "media_source.h"
#include "media_receiver.h"
#include "slog.h"

#include "utils/timing.h"
#include "stat_init.h"

struct media_stat{
	unsigned int frame;
	struct timeval tv_start;

	unsigned int frame_rate;
	unsigned int frame_last;
	struct timeval tv_last;
	
	int (*stat_func)(struct frame_buf *buf, void *arg);
	void (*show_stat)(struct media_stat *stat);

	struct media_receiver *receiver;
	struct media_source *source;
};

static void media_stat_free(struct media_stat *stat);

static struct media_stat * media_stat_new(char *source_name, 
		int (*stat_func)(struct frame_buf *buf, void *arg),
		void (*show_stat)(struct media_stat *stat))
{
	struct media_stat *stat = NULL;
	stat = (struct media_stat *)calloc(1, sizeof(struct media_stat));
	if (!stat) {
		return NULL;
	}

	stat->stat_func = stat_func;
	stat->show_stat = show_stat;
	stat->source = server_media_source_find(source_name);
	if (!stat->source) {
		goto failed;
	}

	stat->receiver = media_receiver_new();
	if (!stat->receiver) {
		goto failed;
	}

	media_receiver_set_callback(stat->receiver, stat->stat_func, stat);
	media_source_add_receiver(stat->source, stat->receiver);

	SLOG(SLOG_INFO, "Stat stream %s", source_name);

	return stat;
failed:
	media_stat_free(stat);
	return NULL;
}

static void media_stat_free(struct media_stat *stat)
{
	if (stat) {
		if (stat->receiver) {
			media_source_del_receiver(stat->receiver);
			media_receiver_free(stat->receiver);
		}
		if (stat->frame > 0 && stat->frame_last > 0) {
			if (stat->show_stat)
				stat->show_stat(stat);
		}
		free(stat);
	}
}

static int h264_stat_func(struct frame_buf *buf, void *arg)
{
	struct media_stat *stat = (struct media_stat *)arg;

	if (stat->frame == 0) {
		stat->tv_start = buf->timestamp;
		stat->tv_last = buf->timestamp;
	}
	
	if (buf->timestamp.tv_sec > stat->tv_last.tv_sec) {
		stat->frame_rate = stat->frame - stat->frame_last;
		stat->frame_last = stat->frame;

		SLOG(SLOG_INFO, "[%s] Framerate: %d", stat->source->name, stat->frame_rate);
	}

	stat->frame++;
	stat->tv_last = buf->timestamp;

	return 0;
}

static void h264_show_stat(struct media_stat *stat)
{
	struct timeval running_time;
	float totaltime = 0.0;
	float fps = 0.0;

	timeval_sub(&(stat->tv_last), &(stat->tv_start), &running_time);
	totaltime = running_time.tv_sec;
	totaltime += (float)running_time.tv_usec / 1000000;
	fps = ((float)stat->frame) / totaltime;
	
	SLOG(SLOG_INFO, "Stream [%s]", stat->source->name);
	SLOG(SLOG_INFO, "    Running Time : %.06f", totaltime);
	SLOG(SLOG_INFO, "    Total Frame  : %d", stat->frame);
	SLOG(SLOG_INFO, "    Frame Rate   : %.06f", fps);
}

static struct media_stat *h264_hd_stat = NULL;
static struct media_stat *h264_vga_stat = NULL;
static struct media_stat *h264_qvga_stat = NULL;
static struct media_stat *h264_qqvga_stat = NULL;

int stat_init()
{
	/* h264 stat */
	h264_hd_stat = media_stat_new("LIVE-H264-HD", h264_stat_func, h264_show_stat);
	h264_vga_stat = media_stat_new("LIVE-H264-VGA", h264_stat_func, h264_show_stat);
	h264_qvga_stat = media_stat_new("LIVE-H264-QVGA", h264_stat_func, h264_show_stat);
	h264_qqvga_stat = media_stat_new("LIVE-H264-QQVGA", h264_stat_func, h264_show_stat);

	return 0;
}

void stat_destroy()
{
	media_stat_free(h264_hd_stat);
	media_stat_free(h264_vga_stat);
	media_stat_free(h264_qvga_stat);
	media_stat_free(h264_qqvga_stat);
}


