#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <event2/event.h>

#include "common.h"
#include "input_sonix_291b.h"
#include "media_source.h"
#include "v4l2_api.h"
#include "sonix_xu_ctrls.h"
#include "slog.h"

int input_sonix_291b_init()
{
	int fd = -1;
	int ret = 0;
	
	if (video_open("/dev/video0", &fd)) {
		SLOG(SLOG_ERROR, "Failed to open video device");
		return -1;
	}

	if (XU_Ctrl_ReadChipID(fd)) {
		SLOG(SLOG_ERROR, "Failed to read chip_id");
		goto failed;
	}

	if (chip_id != CHIP_SNC291B) {
		SLOG(SLOG_ERROR, "This application is only for SONiX 291B!!!");
		goto failed;		
	}

	if (XU_Init_Ctrl(fd)) {
		SLOG(SLOG_ERROR, "Failed to add sonix xu ctrls");
		goto failed;
	}

	if (XU_H264_Set_SEI(fd, 1)) {
		SLOG(SLOG_ERROR, "Failed to enable h264 sei header");
		goto failed;
	}

	// set multi stream to MULTI_STREAM_HD_QVGA
	if(XU_Multi_Set_Type(fd, MULTI_STREAM_HD_QVGA)) {
		SLOG(SLOG_ERROR, "Failed to set multi-stream to MULTI_STREAM_HD_QVGA");
		goto failed;
	}

	// enable h264 multi stream
	if (XU_Multi_Set_Enable(fd, 1) < 0) {
		SLOG(SLOG_ERROR, "Failed to enable multi-stream H264");
		goto failed;
	}

	close(fd);
	
	return 0;
failed:
	if (fd >= 0)
		close(fd);
	return -1;
}

