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

struct recorder{
	char *dirname;
	char *filename;
	char *suffix;
	char fullname[512];
	int sequence;
	
	int fd;

	int (*record_func)(struct frame_buf *buf, void *arg);

	struct media_receiver *receiver;
	struct media_source *source;
};

static void recorder_free(struct recorder *recorder);

static struct recorder * recorder_new(char *source_name, char *dirname, char *filename, char *suffix,
	int (*record_func)(struct frame_buf *buf, void *arg))
{
	if (!source_name || !dirname || !filename || !suffix || !record_func) {
		return NULL;
	}

	struct recorder *recorder = NULL;
	recorder = (struct recorder *)calloc(1, sizeof(struct recorder));
	if (!recorder) {
		return NULL;
	}

	recorder->fd = -1;
	recorder->dirname = strdup(dirname);
	recorder->filename = strdup(filename);
	recorder->suffix = strdup(suffix);
	if (!recorder->dirname || !recorder->filename || !recorder->suffix) {
		goto failed;
	}
	recorder->record_func = record_func;
	recorder->source = server_media_source_find(source_name);
	if (!recorder->source) {
		goto failed;
	}
	
	recorder->receiver = media_receiver_new();
	if (!recorder->receiver) {
		goto failed;
	}

	media_receiver_set_callback(recorder->receiver, recorder->record_func, recorder);
	media_source_add_receiver(recorder->source, recorder->receiver);

	SLOG(SLOG_INFO, "Record %s to file %s.%s in directory %s", 
		source_name, recorder->filename, recorder->suffix, recorder->dirname);
	
	return recorder;
failed:
	recorder_free(recorder);
	return NULL;
}

static void recorder_free(struct recorder *recorder)
{
	if (recorder) {
		if (recorder->receiver) {
			media_source_del_receiver(recorder->receiver);
			media_receiver_free(recorder->receiver);
		}
		if (recorder->dirname) 
			free(recorder->dirname);
		if (recorder->filename) 
			free(recorder->filename);
		if (recorder->suffix) 
			free(recorder->suffix);
		if (recorder->fd >= 0)
			close(recorder->fd);
		free(recorder);
	}
}

static int record_into_single_file(struct frame_buf *buf, void *arg)
{
	struct recorder *recorder = (struct recorder *)arg;
	if (recorder->fd < 0) {
		sprintf(recorder->fullname, "%s/%s.%s", recorder->dirname, recorder->filename, recorder->suffix);
		recorder->fd = open(recorder->fullname, O_CREAT | O_LARGEFILE | O_TRUNC | O_RDWR,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (recorder->fd < 0) {
			SLOG(SLOG_ERROR, "Failed to open file %s", recorder->fullname);
			return -1;
		}
	}
	
	int n_write = 0;
	int ret = 0;
	while (n_write != buf->size) {
		ret = write(recorder->fd, ((char *)buf->data + n_write), buf->size - n_write);
		if (ret < 0) {
			SLOG(SLOG_ERROR, "Failed to write file %s", recorder->fullname);
			break;
		}
		n_write += ret;
	}
	
	return 0;
}

static int record_into_sequence_file(struct frame_buf *buf, void *arg)
{
	struct recorder *recorder = (struct recorder *)arg;
	
	sprintf(recorder->fullname, "%s/%s-%d.%s", recorder->dirname, recorder->filename, 
		recorder->sequence++, recorder->suffix);
	
	recorder->fd = open(recorder->fullname, O_CREAT | O_LARGEFILE | O_TRUNC | O_RDWR,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (recorder->fd < 0) {
		SLOG(SLOG_ERROR, "Failed to open file %s", recorder->fullname);
		return -1;
	}
	
	int n_write = 0;
	int ret = 0;
	while (n_write != buf->size) {
		ret = write(recorder->fd, ((char *)buf->data + n_write), buf->size - n_write);
		if (ret < 0) {
			SLOG(SLOG_ERROR, "Failed to write file %s", recorder->fullname);
			break;
		}
		n_write += ret;
	}

	close(recorder->fd);
	
	return 0;
}

struct recorder *h264_hd_recorder = NULL;
struct recorder *h264_vga_recorder = NULL;
struct recorder *h264_qvga_recorder = NULL;
struct recorder *h264_qqvga_recorder = NULL;

struct recorder *h264_hd_seq_recorder = NULL;
struct recorder *h264_vga_seq_recorder = NULL;
struct recorder *h264_qvga_seq_recorder = NULL;
struct recorder *h264_qqvga_seq_recorder = NULL;

struct recorder *mjpeg_hd_seq_recorder = NULL;
struct recorder *mjpeg_vga_seq_recorder = NULL;
struct recorder *mjpeg_qvga_seq_recorder = NULL;
struct recorder *mjpeg_qqvga_seq_recorder = NULL;

int record_init()
{
	/* h264 record */
	h264_hd_recorder = recorder_new("LIVE-H264-HD", "/tftpboot/record/", 
		"Record_H264_HD_RAW", "h264", record_into_single_file);

	h264_vga_recorder = recorder_new("LIVE-H264-VGA", "/tftpboot/record/", 
		"Record_H264_VGA_RAW", "h264", record_into_single_file);

	h264_qvga_recorder = recorder_new("LIVE-H264-QVGA", "/tftpboot/record/", 
		"Record_H264_QVGA_RAW", "h264", record_into_single_file);

	h264_qqvga_recorder = recorder_new("LIVE-H264-QQVGA", "/tftpboot/record/", 
		"Record_H264_QQVGA_RAW", "h264", record_into_single_file);
	

	/* h264 sequence record 
	h264_hd_seq_recorder = recorder_new("LIVE-H264-HD", "/tftpboot/record/H264-HD/", 
		"Record_H264_HD_RAW", "h264", record_into_sequence_file);

	h264_vga_seq_recorder = recorder_new("LIVE-H264-VGA", "/tftpboot/record/H264-VGA/", 
		"Record_H264_VGA_RAW", "h264", record_into_sequence_file);

	h264_qvga_seq_recorder = recorder_new("LIVE-H264-QVGA", "/tftpboot/record/H264-QVGA/", 
		"Record_H264_QVGA_RAW", "h264", record_into_sequence_file);

	h264_qqvga_seq_recorder = recorder_new("LIVE-H264-QQVGA", "/tftpboot/record/H264-QQVGA/", 
		"Record_H264_QQVGA_RAW", "h264", record_into_sequence_file);
	*/

	/* mjpeg sequence record 
	mjpeg_hd_seq_recorder = recorder_new("LIVE-MJPEG-HD", "/tftpboot/record/MJPEG-HD/", 
		"Record_MJPEG_HD_RAW", "jpg", record_into_sequence_file);

	mjpeg_vga_seq_recorder = recorder_new("LIVE-MJPEG-VGA", "/tftpboot/record/MJPEG-VGA/", 
		"Record_MJPEG_VGA_RAW", "jpg", record_into_sequence_file);

	mjpeg_qvga_seq_recorder = recorder_new("LIVE-MJPEG-QVGA", "/tftpboot/record/MJPEG-QVGA/", 
		"Record_MJPEG_QVGA_RAW", "jpg", record_into_sequence_file);

	mjpeg_qqvga_seq_recorder = recorder_new("LIVE-MJPEG-QQVGA", "/tftpboot/record/MJPEG-QQVGA/", 
		"Record_MJPEG_QQVGA_RAW", "jpg", record_into_sequence_file);
	*/

	return 0;
}

void record_destroy()
{
	recorder_free(h264_hd_recorder);
	recorder_free(h264_vga_recorder);
	recorder_free(h264_qvga_recorder);
	recorder_free(h264_qqvga_recorder);

	recorder_free(h264_hd_seq_recorder);
	recorder_free(h264_vga_seq_recorder);
	recorder_free(h264_qvga_seq_recorder);
	recorder_free(h264_qqvga_seq_recorder);

	recorder_free(mjpeg_hd_seq_recorder);
	recorder_free(mjpeg_vga_seq_recorder);
	recorder_free(mjpeg_qvga_seq_recorder);
	recorder_free(mjpeg_qqvga_seq_recorder);
}


