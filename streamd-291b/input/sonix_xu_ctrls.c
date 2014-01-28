#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "sonix_xu_ctrls.h"

#define LENGTH_OF_SONIX_XU_SYS_CTR		(7)
#define LENGTH_OF_SONIX_XU_USR_CTR		(7)
#define SONIX_SN9C291_SERIES_CHIPID 	0x90
#define SONIX_SN9C292_SERIES_CHIPID		0x92
#define SONIX_SN9C292_DDR_64M			0x00
#define SONIX_SN9C292_DDR_16M			0x03

#define Default_fwLen 13

extern struct H264Format *gH264fmt;
unsigned char m_CurrentFPS = 24;
unsigned char m_CurrentFPS_292 = 30;
unsigned int  chip_id = CHIP_NONE;
const unsigned char Default_fwData[Default_fwLen] = {
	0x05, 0x00, 0x02, 0xD0, 0x01,		// W=1280, H=720, NumOfFrmRate=1
	0xFF, 0xFF, 0xFF, 0xFF,				// Frame size
	0x07, 0xA1, 0xFF, 0xFF,				// 20
};

static struct uvc_xu_control_info sonix_xu_sys_ctrls[] = 
{
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_ASIC_RW,
		.index    = 0,
		.size     = 4,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | 
		            UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_FRAME_INFO,
		.index    = 1,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | 
		            UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_H264_CTRL,
		.index    = 2,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_MJPG_CTRL,
		.index    = 3,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_OSD_CTRL,
		.index    = 4,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | 
		            UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_MOTION_DETECTION,
		.index    = 5,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_IMG_SETTING,
		.index    = 6,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
	},
};

static struct uvc_xu_control_info sonix_xu_usr_ctrls[] =
{
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_FRAME_INFO,
      .index    = 0,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX |
                  UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_H264_CTRL,
      .index    = 1,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_MJPG_CTRL,
      .index    = 2,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_OSD_CTRL,
      .index    = 3,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX |
                  UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_MOTION_DETECTION,
      .index    = 4,
      .size     = 24,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_IMG_SETTING,
      .index    = 5,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_MULTI_STREAM_CTRL,
      .index    = 6,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },	
};

//  SONiX XU system Ctrls Mapping
static struct uvc_xu_control_mapping sonix_xu_sys_mappings[] = 
{
	{
		.id        = V4L2_CID_ASIC_RW_SONIX,
		.name      = "SONiX: Asic Read",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_ASIC_RW,
		.size      = 4,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_SIGNED
	},
	{
		.id        = V4L2_CID_FRAME_INFO_SONIX,
		.name      = "SONiX: H264 Format",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_FRAME_INFO,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_RAW
	},
	{
		.id        = V4L2_CID_H264_CTRL_SONIX,
		.name      = "SONiX: H264 Control",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_H264_CTRL,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
	},
	{
		.id        = V4L2_CID_MJPG_CTRL_SONIX,
		.name      = "SONiX: MJPG Control",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_MJPG_CTRL,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
	},
	{
		.id        = V4L2_CID_OSD_CTRL_SONIX,
		.name      = "SONiX: OSD Control",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_OSD_CTRL,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_RAW
	},
	{
		.id        = V4L2_CID_MOTION_DETECTION_SONIX,
		.name      = "SONiX: Motion Detection",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_MOTION_DETECTION,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
	},
	{
		.id        = V4L2_CID_IMG_SETTING_SONIX,
		.name      = "SONiX: Image Setting",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_IMG_SETTING,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
	},
};

// SONiX XU user Ctrls Mapping
static struct uvc_xu_control_mapping sonix_xu_usr_mappings[] =
{
    {
      .id        = V4L2_CID_FRAME_INFO_SONIX,
      .name      = "SONiX: H264 Format",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_FRAME_INFO,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_RAW
    },
    {
      .id        = V4L2_CID_H264_CTRL_SONIX,
      .name      = "SONiX: H264 Control",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_H264_CTRL,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_MJPG_CTRL_SONIX,
      .name      = "SONiX: MJPG Control",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_MJPG_CTRL,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_OSD_CTRL_SONIX,
      .name      = "SONiX: OSD Control",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_OSD_CTRL,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_RAW
    },
    {
      .id        = V4L2_CID_MOTION_DETECTION_SONIX,
      .name      = "SONiX: Motion Detection",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_MOTION_DETECTION,
      .size      = 24,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_IMG_SETTING_SONIX,
      .name      = "SONiX: Image Setting",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_IMG_SETTING,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_MULTI_STREAM_CTRL_SONIX,
      .name      = "SONiX: Multi Stram Control ",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_MULTI_STREAM_CTRL,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },	
};

int XU_Ctrl_Add(int fd, struct uvc_xu_control_info *info, struct uvc_xu_control_mapping *map) 
{
	int ret = 0;
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_ADD, info)) < 0 ) {
		if (errno == EEXIST) {	
			return 0;
		} else if (errno == EACCES) {
			printf("Need root previledges for adding extension unit(XU) controls\n");
			return -1;
		} else {
			perror("Failed to add xu ctrls");
			return -1;
		}
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_MAP, map)) < 0) {
		if (errno == EEXIST) {	
			return 0;
		} else if (errno == EACCES) {
			printf("Need admin previledges for adding extension unit(XU) controls\n");
			return -1;
		} else {
			perror("Failed to mapping xu ctrls");
			return -1;
		}
	}
	
	return 0;
}

int XU_Init_Ctrl(int fd) 
{
	int i = 0;
	int ret = 0;
	int length = 0;
	struct uvc_xu_control_info *xu_infos = NULL;
	struct uvc_xu_control_mapping *xu_mappings = NULL;
	
	// Add xu READ ASIC first
	ret = XU_Ctrl_Add(fd, &sonix_xu_sys_ctrls[i], &sonix_xu_sys_mappings[i]);
	if (ret)
		return -1;

	// Read chip ID
	ret = XU_Ctrl_ReadChipID(fd);
	if (ret) 
		return -1;

	// Decide which xu set had been add
	if (chip_id == CHIP_SNC291A)	{
		xu_infos = sonix_xu_sys_ctrls;
		xu_mappings = sonix_xu_sys_mappings;
		i = 1;
		length = LENGTH_OF_SONIX_XU_SYS_CTR;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xu_infos = sonix_xu_usr_ctrls;
		xu_mappings = sonix_xu_usr_mappings;
		i = 0;
		length = LENGTH_OF_SONIX_XU_USR_CTR;
	} else {
		printf("Unknown chip id 0x%x\n", chip_id);
		return -1;
	}
	
	// Add other xu accroding chip ID
	for (; i < length; i++) {
		ret= XU_Ctrl_Add(fd, &xu_infos[i], &xu_mappings[i]);
		if (ret < 0)
			break;
	} 
	
	return 0;
}

int XU_Ctrl_ReadChipID(int fd)
{
	int ret = 0;
	__u8 ctrldata[4];

	struct uvc_xu_control xctrl = {
		.unit 		= XU_SONIX_SYS_ID,						/* bUnitID 				*/
		.selector 	= XU_SONIX_SYS_ASIC_RW,					/* function selector	*/
		.size 		= sizeof(ctrldata),						/* data size			*/
		.data 		= ctrldata								/* *data				*/
	};
	
	xctrl.data[0] = 0x1f;
	xctrl.data[1] = 0x10;
	xctrl.data[2] = 0x0;
	xctrl.data[3] = 0xFF;		/* Dummy Write */
	
	/* Dummy Write */
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("Failed to read chip id - %d\n", ret);
		return -1;
	}

	/* Asic Read */
	xctrl.data[3] = 0x00;
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("Failed to read chip id - %d\n", ret);
		return -1;
	}
	
	if (xctrl.data[2] == SONIX_SN9C291_SERIES_CHIPID) {
		chip_id = CHIP_SNC291A;
	} else if (xctrl.data[2] == SONIX_SN9C292_SERIES_CHIPID) {
		xctrl.data[0] = 0x07;		//DRAM SIZE
		xctrl.data[1] = 0x16;
		xctrl.data[2] = 0x0;
		xctrl.data[3] = 0xFF;		/* Dummy Write */

		/* Dummy Write */
		if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
			printf("Failed to read chip dram size - %d\n", ret);
			return -1;
		}

		/* Asic Read */
		xctrl.data[3] = 0x00;
		if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
			printf("Failed to read chip dram size - %d\n", ret);
			return -1;
		}
		
		if (xctrl.data[2] == SONIX_SN9C292_DDR_64M)
			chip_id = CHIP_SNC292A;
		else if (xctrl.data[2] == SONIX_SN9C292_DDR_16M)
			chip_id = CHIP_SNC291B;			
	}
	
	return 0;
}

int H264_GetFormat(int fd)
{
	int i,j;
	int ret = 0;
	int iH264FormatCount = 0;
	
	unsigned char *fwData = NULL;
	unsigned short fwLen = 0;

	// Init H264 XU Ctrl Format
	ret = XU_H264_InitFormat(fd);
	if (ret < 0)	{
		printf("H264 XU Ctrl Format failed, use default Format\n");
		fwLen = Default_fwLen;
		fwData = (unsigned char *)calloc(fwLen,sizeof(unsigned char));
		memcpy(fwData, Default_fwData, fwLen);
		goto Skip_XU_GetFormat;
	}

	// Probe : Get format through XU ctrl
	ret = XU_H264_GetFormatLength(fd, &fwLen);
	if (ret || fwLen <= 0) {
		return -1;
	}
	
	// alloc memory
	fwData = (unsigned char *)calloc(fwLen,sizeof(unsigned char));
	ret = XU_H264_GetFormatData(fd, fwData,fwLen);
	if (ret)	{
		return -1;
	}

Skip_XU_GetFormat :
	// Get H.264 format count
	iH264FormatCount = H264_CountFormat(fwData, fwLen);
	if (iH264FormatCount > 0) {
		printf("H264_GetFormat ==> FormatCount : %d \n", iH264FormatCount);
		gH264fmt = (struct H264Format *)malloc(sizeof(struct H264Format)*iH264FormatCount);
	} else {
		printf("Get Format count Data failed\n");
		return -1;
	}

	// Parse & Save Size/Framerate into structure
	ret = H264_ParseFormat(fwData, fwLen, gH264fmt);
	if (ret) {
		printf("Parse Format Data failed\n");
		return -1;
	}
	
	for (i = 0; i < iH264FormatCount; i++) {
		printf("Format index: %d --- (%d x %d) ---\n", i+1, gH264fmt[i].wWidth, gH264fmt[i].wHeight);
		for (j = 0; j < gH264fmt[i].fpsCnt; j++) {
			if (chip_id == CHIP_SNC291A)	{
				printf("(%d) %2d fps\n", j+1, H264_GetFPS(gH264fmt[i].FrPay[j]));
			} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
				printf("(%d) %2d fps\n", j+1, H264_GetFPS(gH264fmt[i].FrPay[j*2]));
			}
		}
	}

	if (fwData) {
		free(fwData);
		fwData = NULL;
	}
	
	return 0;
}

int H264_CountFormat(unsigned char *Data, int len)
{
	int fmtCnt = 0;
	int cur_len = 0;
	int cur_fpsNum = 0;
	
	if (Data == NULL || len == 0)
		return 0;

	// count Format numbers
	while (cur_len < len) {
		cur_fpsNum = Data[cur_len+4];
		
		if (chip_id == CHIP_SNC291A) {
			cur_len += 9 + cur_fpsNum * 4;
		} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
			cur_len += 9 + cur_fpsNum * 6;
		}

		fmtCnt++;
	}
	
	if (cur_len != len) {
		printf("H264_CountFormat ==> cur_len = %d, fwLen= %d\n", cur_len , len);
		return 0;
	}
	
	return fmtCnt;
}

int H264_ParseFormat(unsigned char *Data, int len, struct H264Format *fmt)
{
	int cur_len = 0;
	int cur_fmtid = 0;
	int cur_fpsNum = 0;
	int i;

	while (cur_len < len) {
		// Copy Size
		fmt[cur_fmtid].wWidth  = ((unsigned short)Data[cur_len]<<8)   + (unsigned short)Data[cur_len+1];
		fmt[cur_fmtid].wHeight = ((unsigned short)Data[cur_len+2]<<8) + (unsigned short)Data[cur_len+3];
		fmt[cur_fmtid].fpsCnt  = Data[cur_len+4];
		fmt[cur_fmtid].FrameSize =	((unsigned int)Data[cur_len+5] << 24) | 
						((unsigned int)Data[cur_len+6] << 16) | 
						((unsigned int)Data[cur_len+7] << 8 ) | 
						((unsigned int)Data[cur_len+8]);

		printf("Data[5~8]: 0x%02x%02x%02x%02x \n", Data[cur_len+5],Data[cur_len+6],Data[cur_len+7],Data[cur_len+8]);
		printf("fmt[%d].FrameSize: 0x%08x \n", cur_fmtid, fmt[cur_fmtid].FrameSize);

		// Alloc memory for Frame rate 
		cur_fpsNum = Data[cur_len+4];
		
		if (chip_id == CHIP_SNC291A) {
			fmt[cur_fmtid].FrPay = (unsigned int *)malloc(sizeof(unsigned int)*cur_fpsNum);
		} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A))	{
			fmt[cur_fmtid].FrPay = (unsigned int *)malloc(sizeof(unsigned int)*cur_fpsNum*2);
		}
		
		for (i = 0; i < cur_fpsNum; i++) {
			if (chip_id == CHIP_SNC291A) {
				fmt[cur_fmtid].FrPay[i] = (unsigned int)Data[cur_len+9+i*4] << 24 | 
								(unsigned int)Data[cur_len+9+i*4+1] << 16 |
								(unsigned int)Data[cur_len+9+i*4+2] << 8  |
								(unsigned int)Data[cur_len+9+i*4+3];			
				//printf("fmt[cur_fmtid].FrPay[%d]: 0x%08x \n", i, fmt[cur_fmtid].FrPay[i]);
			} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
				fmt[cur_fmtid].FrPay[i*2] =	(unsigned int)Data[cur_len+9+i*6] << 8 | (unsigned int)Data[cur_len+9+i*6+1];
				fmt[cur_fmtid].FrPay[i*2+1] = (unsigned int)Data[cur_len+9+i*6+2] << 24 | 
								(unsigned int)Data[cur_len+9+i*6+3] << 16 |
								(unsigned int)Data[cur_len+9+i*6+4] << 8  |
								(unsigned int)Data[cur_len+9+i*6+5] ;
				printf("fmt[cur_fmtid].FrPay[%d]: 0x%04x  0x%08x \n", i, fmt[cur_fmtid].FrPay[i*2], fmt[cur_fmtid].FrPay[i*2+1]);
			}
		}
		
		// Do next format
		if (chip_id == CHIP_SNC291A) {
			cur_len += 9 + cur_fpsNum * 4;
		} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
			cur_len += 9 + cur_fpsNum * 6;
		}
		cur_fmtid++;
	}
	
	if (cur_len != len) {
		printf("H264_ParseFormat <==  fail \n");
		return -1;
	}

	return 0;
}

int H264_GetFPS(unsigned int FrPay)
{
	int fps = 0;

	if (chip_id == CHIP_SNC291A) {
		unsigned short frH = (FrPay & 0xFF000000) >> 16;
		unsigned short frL = (FrPay & 0x00FF0000) >> 16;
		unsigned short fr = (frH|frL);
		fps = ((unsigned int)10000000 / fr) >> 8;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		fps = ((unsigned int)10000000 / (unsigned short)FrPay) >> 8;
	}

	return fps;
}

int XU_H264_InitFormat(int fd)
{
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_FRAME_INFO;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_FRAME_INFO;
	}
		
	// Switch command : FMT_NUM_INFO
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x01;

	// TODO: ?? spec says that set_cur is not supportted
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("   Set Switch command : FMT_NUM_INFO FAILED (%i)\n", ret);
		return -1;
	}
	
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {	
		printf("   ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}
	
	return 0;
}

int XU_H264_GetFormatLength(int fd, unsigned short *fwLen)
{
	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_FRAME_INFO;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_FRAME_INFO;
	}
		
	// Switch command : FMT_DATA_LENGTH_INFO
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x02;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("   Set Switch command : FMT_Data Length_INFO FAILED (%i)\n", ret);
		return -1;
	}

	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("   ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	for (i = 0; i < 11; i+=2)
		printf(" Get Data[%d] = 0x%x\n", i, (xctrl.data[i]<<8)+xctrl.data[i+1]);

	// Get H.264 format length
	*fwLen = ((unsigned short)xctrl.data[6]<<8) + xctrl.data[7];
	printf(" H.264 format Length = 0x%x\n", *fwLen);

	return 0;
}

int XU_H264_GetFormatData(int fd, unsigned char *fwData, unsigned short fwLen)
{
	int ret = 0;
	__u8 ctrldata[11]={0};

	int loop = 0;
	int LoopCnt  = (fwLen%11) ? (fwLen/11)+1 : (fwLen/11) ;
	int Copyleft = fwLen;

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};
	
	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_FRAME_INFO;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_FRAME_INFO;
	}
	
	// Switch command FRM_DATA_INFO
	xctrl.data[0] = 0x9A;		
	xctrl.data[1] = 0x03;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0)	{
		printf("   Set Switch command : FRM_DATA_INFO FAILED (%i)\n", ret);
		return -1;
	}

	// Get H.264 Format Data
	xctrl.data[0] = 0x02;		// Stream: 1
	xctrl.data[1] = 0x01;		// Format: 1 (H.264)

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) >= 0) {
		printf("   Set Switch command : FRM_DATA_INFO FAILED (%i)\n", ret);
		return -1;
	}

	// Read all H.264 format data
	for (loop = 0; loop < LoopCnt; loop++) {
		memset(xctrl.data, 0, xctrl.size);
		if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) >= 0) {
			// Copy Data
			if (Copyleft >= 11) {
				memcpy(&fwData[loop*11] , xctrl.data, 11);
				Copyleft -= 11;
			} else {
				memcpy(&fwData[loop*11] , xctrl.data, Copyleft);
				Copyleft = 0;
			}
		} else {
			printf("   ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
			return -1;
		}
	}

	return 0;
}

int XU_H264_SetFormat(int fd, struct Cur_H264Format fmt)
{
	// Need to get H264 format first
	if (gH264fmt == NULL) {
		printf("SONiX_UVC_TestAP @XU_H264_SetFormat : Do XU_H264_GetFormat before setting H264 format\n");
		return -1;
	}

	if (chip_id == CHIP_SNC291A) {
		printf("XU_H264_SetFormat ==> %d-%d => (%d x %d):%d fps\n", 
				fmt.FmtId+1, fmt.FrameRateId+1, 
				gH264fmt[fmt.FmtId].wWidth, 
				gH264fmt[fmt.FmtId].wHeight, 
				H264_GetFPS(gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId]));
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		printf("XU_H264_SetFormat ==> %d-%d => (%d x %d):%d fps\n", 
				fmt.FmtId+1, fmt.FrameRateId+1, 
				gH264fmt[fmt.FmtId].wWidth, 
				gH264fmt[fmt.FmtId].wHeight, 
				H264_GetFPS(gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2]));
	}

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_FRAME_INFO;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_FRAME_INFO;
	}
		
	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x21;				// Commit_INFO
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_FMT ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set H.264 format setting data
	xctrl.data[0] = 0x02;				// Stream : 1
	// TODO: get h264 stream index by FMT_NUM_INFO
	xctrl.data[1] = 0x01;				// Format index : 1 (H.264) 
	xctrl.data[2] = fmt.FmtId + 1;		// Frame index (Resolution index), firmware index starts from 1
//	xctrl.data[3] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0xFF000000) >> 24;	// Frame interval
//	xctrl.data[4] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x00FF0000) >> 16;
	xctrl.data[5] = (gH264fmt[fmt.FmtId].FrameSize & 0x00FF0000) >> 16;
	xctrl.data[6] = (gH264fmt[fmt.FmtId].FrameSize & 0x0000FF00) >> 8;
	xctrl.data[7] = (gH264fmt[fmt.FmtId].FrameSize & 0x000000FF);
//	xctrl.data[8] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x0000FF00) >> 8;	// padload size
//	xctrl.data[9] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x000000FF) ;

	if (chip_id == CHIP_SNC291A)	{
		xctrl.data[3] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0xFF000000) >> 24;	// Frame interval
		xctrl.data[4] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x00FF0000) >> 16;
		xctrl.data[8] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x0000FF00) >> 8;
		xctrl.data[9] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x000000FF);
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.data[3] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2] & 0x0000FF00) >> 8;	// Frame interval
		xctrl.data[4] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2] & 0x000000FF);
		xctrl.data[8] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2+1] & 0xFF000000) >> 24;
		xctrl.data[9] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2+1] & 0x00FF0000) >> 16;
		xctrl.data[10] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2+1] & 0x0000FF00) >> 8;
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_FMT ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		memset(xctrl.data, 0, xctrl.size);
		xctrl.data[0] = (gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2+1] & 0x000000FF);

		if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
			printf("XU_H264_Set_FMT____2 ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
			return -1;
		}
	}	

	return 0;
}

int XU_H264_Get_Mode(int fd, int *Mode)
{	
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};
	
	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;				// H264_ctrl_type
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x06;				// H264_mode
	}
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Get_Mode ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// Get mode
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_H264_Get_Mode ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}
	
	*Mode = xctrl.data[0];
	
	printf("XU_H264_Get_Mode (%s)<==\n", *Mode==1?"CBR mode":(*Mode==2?"VBR mode":"error"));
	
	return 0;
}

int XU_H264_Set_Mode(int fd, int Mode)
{	
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;				// H264_ctrl_type
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x06;				// H264_mode
	}
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_Mode ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// Set CBR/VBR Mode
	memset(xctrl.data, 0, xctrl.size);
	xctrl.data[0] = Mode;
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_Mode ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	return 0;

}
// TODO:                
int XU_H264_Get_QP_Limit(int fd, int *QP_Min, int *QP_Max)
{
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;				// H264_limit
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;				// H264_limit
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Get_QP_Limit ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	memset(xctrl.data, 0, xctrl.size);
	// Get QP value
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_H264_Get_QP_Limit ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	*QP_Min = xctrl.data[0];
	*QP_Max = xctrl.data[1];
	
	printf("XU_H264_Get_QP_Limit (0x%x, 0x%x)<==\n", *QP_Min, *QP_Max);
	
	return 0;

}

int XU_H264_Get_QP(int fd, int *QP_Val)
{
	int ret = 0;
	__u8 ctrldata[11]={0};
	
	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};
	int qp_min, qp_max;
	
	*QP_Val = -1;
	if (XU_H264_Get_QP_Limit(fd, &qp_min, &qp_max)) {
		return -1;
	}

	if (chip_id == CHIP_SNC291A)	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x05;				// H264_VBR_QP
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x07;				// H264_QP
	}
	
	if ((ret  = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Get_QP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// Get QP value
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_H264_Get_QP ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	*QP_Val = xctrl.data[0];
	
	printf("XU_H264_Get_QP (0x%x)<==\n", *QP_Val);
	return 0;

}

int XU_H264_Set_QP(int fd, int QP_Val)
{
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x05;				// H264_VBR_QP
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x07;				// H264_QP	
	}
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Get_QP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// Set QP value
	memset(xctrl.data, 0, xctrl.size);
	xctrl.data[0] = QP_Val;
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_QP ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	return 0;

}

int XU_H264_Get_BitRate(int fd, double *BitRate)
{
	int ret = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;
	*BitRate = -1.0;

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;				// H264_BitRate
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;				// H264_BitRate
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Get_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// Get Bit rate ctrl number
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_H264_Get_BitRate ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}
	
	if (chip_id == CHIP_SNC291A) {
		BitRate_CtrlNum = ( xctrl.data[0]<<8 )| (xctrl.data[1]) ;
		// Bit Rate = BitRate_Ctrl_Num*512*fps*8 /1000(Kbps)
		*BitRate = (double)(BitRate_CtrlNum*512.0*m_CurrentFPS*8)/1000.0;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		BitRate_CtrlNum =  ( xctrl.data[0]<<16 )| ( xctrl.data[1]<<8 )| (xctrl.data[2]) ;
		*BitRate = BitRate_CtrlNum;
	}	
	
	printf("XU_H264_Get_BitRate (%.2f)<==\n", *BitRate);
	return 0;
}

int XU_H264_Set_BitRate(int fd, double BitRate)
{
	int ret = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;				// H264_BitRate
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;				// H264_BitRate
	}
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set Bit Rate Ctrl Number
	if (chip_id == CHIP_SNC291A) {
		// Bit Rate = BitRate_Ctrl_Num*512*fps*8/1000 (Kbps)
		BitRate_CtrlNum = (int)((BitRate*1000)/(512*m_CurrentFPS*8));
		xctrl.data[0] = (BitRate_CtrlNum & 0xFF00)>>8;	// BitRate ctrl Num
		xctrl.data[1] = (BitRate_CtrlNum & 0x00FF);
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		// Bit Rate = BitRate_Ctrl_Num*512*fps*8/1000 (Kbps)
		xctrl.data[0] = ((int)BitRate & 0x00FF0000)>>16;
		xctrl.data[1] = ((int)BitRate & 0x0000FF00)>>8;
		xctrl.data[2] = ((int)BitRate & 0x000000FF);
	}	
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_BitRate ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	printf("XU_H264_Set_BitRate <== Success \n");
	return -1;
}

int XU_H264_Set_IFRAME(int fd)
{
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x06;				// H264_IFRAME
	}else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)){
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x04;				// H264_IFRAME
	}
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_IFRAME ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// Set IFrame reset
	xctrl.data[0] = 1;
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_IFRAME ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}	

	return 0;
}

int XU_H264_Get_SEI(int fd, unsigned char *SEI)
{
	int ret = 0;
	__u8 ctrldata[11]={0};

	if (chip_id == CHIP_SNC291A) {
		printf(" ==SN9C290 no support get SEI==\n");	
		return 0;
	}

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_H264_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x05;				// H264_SEI

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Get_SEI ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// Get SEI
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_H264_Get_SEI ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}
	
	*SEI = xctrl.data[0];

	return 0;
}

int XU_H264_Set_SEI(int fd, unsigned char SEI)
{	
	printf("XU_H264_Set_SEI ==>\n");
	int ret = 0;
	__u8 ctrldata[11]={0};

	if (chip_id == CHIP_SNC291A){
		printf(" ==SN9C290 no support Set SEI==\n");	
		return 0;
	}

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_H264_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x05;				// H264_SEI
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_SEI ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set SEI
	xctrl.data[0] = SEI;
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_SEI ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	return 0;
}

int XU_H264_Get_GOP(int fd, unsigned int *GOP)
{
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_H264_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// H264_GOP

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Get_GOP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// Get GOP
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_H264_Get_GOP ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}
	
	*GOP = (xctrl.data[1] << 8) | xctrl.data[0];

	return 0;
}

int XU_H264_Set_GOP(int fd, unsigned int GOP)
{
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_H264_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// H264_GOP
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_GOP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set GOP
	xctrl.data[0] = (GOP & 0xFF);
	xctrl.data[1] = (GOP >> 8) & 0xFF;
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_H264_Set_GOP ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	return 0;
}

int XU_Get(int fd, struct uvc_xu_control *xctrl)
{
	int i = 0;
	int ret = 0;

	// XU Set
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, xctrl)) < 0) {
		printf("XU Get ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// XU Get
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, xctrl)) < 0) {
		printf("XU Get ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}
	
	for(i=0; i<xctrl->size; i++)
		printf("      Get data[%d] : 0x%x\n", i, xctrl->data[i]);
	
	return 0;
}

int XU_Set(int fd, struct uvc_xu_control xctrl)
{
	int i = 0;
	int ret = 0;

	// XU Set
	for(i=0; i<xctrl.size; i++)
		printf("      Set data[%d] : 0x%x\n", i, xctrl.data[i]);
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU Set ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	return 0;
}

int XU_Asic_Read(int fd, unsigned int Addr, unsigned char *AsicData)
{
	int ret = 0;
	__u8 ctrldata[4];

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_SYS_ID,						/* bUnitID				*/
		.selector = XU_SONIX_SYS_ASIC_RW,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	xctrl.data[0] = (Addr & 0xFF);
	xctrl.data[1] = ((Addr >> 8) & 0xFF);
	xctrl.data[2] = 0x0;
	xctrl.data[3] = 0xFF;		/* Dummy Write */
	
	/* Dummy Write */
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("   ioctl(UVCIOC_CTRL_SET) FAILED (%i) \n", ret);
		return -1;
	}
	
	/* Asic Read */
	xctrl.data[3] = 0x00;
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("   ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}
	*AsicData = xctrl.data[2];
	
	return 0;
}

int XU_Asic_Write(int fd, unsigned int Addr, unsigned char AsicData)
{
	int ret = 0;
	__u8 ctrldata[4];

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_SYS_ID,						/* bUnitID				*/
		.selector = XU_SONIX_SYS_ASIC_RW,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};
	
	xctrl.data[0] = (Addr & 0xFF);			/* Addr Low */
	xctrl.data[1] = ((Addr >> 8) & 0xFF);	/* Addr High */
	xctrl.data[2] = AsicData;
	xctrl.data[3] = 0x0;					/* Normal Write */
	
	/* Normal Write */
	if ((ret=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("   ioctl(UVCIOC_CTRL_SET) FAILED (%i) \n", ret);
		return -1;
	}
	
	return 0;
}

int XU_Multi_Get_status(int fd, struct Multistream_Info *status)
{
	printf("XU_Multi_Get_status ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MULTI_STREAM_CTRL,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};
	
	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x01;				// Multi-Stream Status
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Get_status ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get status
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_Multi_Get_status ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	status->strm_type = xctrl.data[0];
	status->format = ( xctrl.data[1]<<8 )| (xctrl.data[2]) ;

	printf("   == XU_Multi_Get_status Success == \n");
	printf("      Get strm_type %d\n", status->strm_type);
	printf("      Get cur_format %d\n", status->format);

	return 0;
}

int XU_Multi_Get_Info(int fd, struct Multistream_Info *Info)
{	
	printf("XU_Multi_Get_Info ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MULTI_STREAM_CTRL,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;				// Multi-Stream Stream Info
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Get_Info ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get Info
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_Multi_Get_Info ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	Info->strm_type = xctrl.data[0];
	Info->format = ( xctrl.data[1]<<8 )| (xctrl.data[2]) ;

	printf("   == XU_Multi_Get_Info Success == \n");
	printf("      Get Support Stream %d\n", Info->strm_type);
	printf("      Get Support Resolution %d\n", Info->format);

	return 0;
}

int XU_Multi_Set_Type(int fd, unsigned int format)
{	
	printf("XU_Multi_Set_Type (%d) ==>\n",format);

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MULTI_STREAM_CTRL,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_Type ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	if (chip_id == CHIP_SNC291B && (format==4 ||format==8 ||format==16)) {
		return -1;
	}
	

	// Set format
	xctrl.data[0] = format;
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_Type ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_Multi_Set_Type <== Success \n");
	return 0;

}

int XU_Multi_Set_Enable(int fd, unsigned char enable)
{	
	printf("XU_Multi_Set_Enable ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MULTI_STREAM_CTRL,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// Enable Multi-Stream Flag

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_Enable ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set enable / disable multistream
	xctrl.data[0] = enable;
	xctrl.data[1] = 0;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_Enable ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	printf("Set H264_Multi_Enable = %d \n",(xctrl.data[0] &0x01));
	printf("Set MJPG_Multi_Enable = %d \n",((xctrl.data[0] >> 1) &0x01));
	printf("XU_Multi_Set_Enable <== Success \n");
	return 0;
}

int XU_Multi_Get_Enable(int fd, unsigned char *enable)
{	
	printf("XU_Multi_Get_Enable ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MULTI_STREAM_CTRL,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// Enable Multi-Stream Flag
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Get_Enable ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get Enable
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_Multi_Get_Enable ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	*enable = xctrl.data[0];

	printf("      Get H264 Multi Stream Enable = %d\n", xctrl.data[0] & 0x01);
	printf("      Get MJPG Multi Stream Enable =  %d\n", (xctrl.data[0] >> 1) & 0x01);	
	printf("XU_Multi_Get_Enable <== Success\n");
	return 0;
}

#if 0
int XU_Multi_Set_BitRate(int fd, unsigned int BitRate1, unsigned int BitRate2, unsigned int BitRate3)
{	
	printf("XU_Multi_Set_BitRate  BiteRate1=%d  BiteRate2=%d  BiteRate3=%d   ==>\n",BitRate1, BitRate2, BitRate3);

	int ret = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MULTI_STREAM_CTRL,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x04;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set BitRate1~3  (unit:bps)
	xctrl.data[0] = (BitRate1 >> 16)&0xFF;
	xctrl.data[1] = (BitRate1 >> 8)&0xFF;
	xctrl.data[2] = BitRate1&0xFF;
	xctrl.data[3] = (BitRate2 >> 16)&0xFF;
	xctrl.data[4] = (BitRate2 >> 8)&0xFF;
	xctrl.data[5] = BitRate2&0xFF;
	xctrl.data[6] = (BitRate3 >> 16)&0xFF;
	xctrl.data[7] = (BitRate3 >> 8)&0xFF;
	xctrl.data[8] = BitRate3&0xFF;
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_BitRate ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_Multi_Set_BitRate <== Success \n");
	return 0;
}

int XU_Multi_Get_BitRate(int fd)
{	
	printf("XU_Multi_Get_BitRate  ==>\n");

	int i = 0;
	int ret = 0;
	int BitRate1 = 0;
	int BitRate2 = 0;
	int BitRate3 = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MULTI_STREAM_CTRL,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x04;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Get_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get BitRate1~3  (unit:bps)
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_Multi_Get_BitRate ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_Multi_Get_BitRate <== Success \n");
	
	for(i=0; i<9; i++)
		printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	BitRate1 =  ( xctrl.data[0]<<16 )| ( xctrl.data[1]<<8 )| (xctrl.data[2]) ;
	BitRate2 =  ( xctrl.data[3]<<16 )| ( xctrl.data[4]<<8 )| (xctrl.data[5]) ;
	BitRate3 =  ( xctrl.data[6]<<16 )| ( xctrl.data[7]<<8 )| (xctrl.data[8]) ;
	
	printf("  HD BitRate (%d)\n", BitRate1);
	printf("  QVGA BitRate (%d)\n", BitRate2);
	printf("  QQVGA BitRate (%d)\n", BitRate3);

	return 0;
}
#endif

int XU_Multi_Set_BitRate(int fd, unsigned int StreamID, unsigned int BitRate)
{	
	printf("XU_Multi_Set_BitRate  StreamID=%d  BiteRate=%d  ==>\n",StreamID ,BitRate);

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MULTI_STREAM_CTRL,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x04;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set BitRate  (unit:bps)
	xctrl.data[0] = StreamID;
	xctrl.data[1] = (BitRate >> 16)&0xFF;
	xctrl.data[2] = (BitRate >> 8)&0xFF;
	xctrl.data[3] = BitRate&0xFF;
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_BitRate ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_Multi_Set_BitRate <== Success \n");
	return 0;
}

int XU_Multi_Get_BitRate(int fd, unsigned int StreamID, unsigned int *BitRate)
{	
	printf("XU_Multi_Get_BitRate  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MULTI_STREAM_CTRL,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x05;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Get_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set Stream ID
	xctrl.data[0] = StreamID;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Get_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get BitRate (unit:bps)
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_Multi_Get_BitRate ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_Multi_Get_BitRate <== Success \n");
	
	for(i=0; i<4; i++)
		printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*BitRate = ( xctrl.data[1]<<16 ) | ( xctrl.data[2]<<8 ) | xctrl.data[3];
	printf("  Stream= %d   BitRate= %d\n", xctrl.data[0], *BitRate);

	return 0;
}

int XU_Multi_Set_QP(int fd, unsigned int StreamID, unsigned int QP_Val)
{	
	printf("XU_Multi_Set_QP  StreamID=%d  QP_Val=%d  ==>\n",StreamID ,QP_Val);

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MULTI_STREAM_CTRL,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x05;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_QP ==> Switch cmd(5) : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set Stream ID
	xctrl.data[0] = StreamID;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_QP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x06;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_QP ==> Switch cmd(6) : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set QP
	xctrl.data[0] = StreamID;
	xctrl.data[1] = QP_Val;

	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Set_QP ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_Multi_Set_QP <== Success \n");
	return 0;
}

int XU_Multi_Get_QP(int fd, unsigned int StreamID, unsigned int *QP_val)
{	
	printf("XU_Multi_Get_QP  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MULTI_STREAM_CTRL,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x05;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Get_QP ==> Switch cmd(5) : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set Stream ID
	xctrl.data[0] = StreamID;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Get_QP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x06;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_Multi_Get_QP ==> Switch cmd(6) : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get QP
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_Multi_Get_QP ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_Multi_Get_QP <== Success \n");
	
	for(i=0; i<2; i++)
		printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*QP_val = xctrl.data[1];
	printf("  Stream= %d   QP_val = %d\n", xctrl.data[0], *QP_val);

	return 0;
}

int XU_OSD_Timer_Ctrl(int fd, unsigned char enable)
{	
	printf("XU_OSD_Timer_Ctrl  ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x00;				// OSD Timer control

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Timer_Ctrl ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set enable / disable timer count
	xctrl.data[0] = enable;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Timer_Ctrl ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_OSD_Timer_Ctrl <== Success \n");

	return 0;
}

int XU_OSD_Set_RTC(int fd, unsigned int year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second)
{	
	printf("XU_OSD_Set_RTC  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x01;				// OSD RTC control

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_RTC ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set RTC
	xctrl.data[0] = second;
	xctrl.data[1] = minute;
	xctrl.data[2] = hour;
	xctrl.data[3] = day;
	xctrl.data[4] = month;
	xctrl.data[5] = (year & 0xFF00) >> 8;
	xctrl.data[6] = (year & 0x00FF);

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_RTC ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	for(i=0; i<7; i++)
		printf("      Set data[%d] : 0x%x\n", i, xctrl.data[i]);
	
	printf("XU_OSD_Set_RTC <== Success \n");

	return 0;
}

int XU_OSD_Get_RTC(int fd, unsigned int *year, unsigned char *month, unsigned char *day, unsigned char *hour, unsigned char *minute, unsigned char *second)
{	
	printf("XU_OSD_Get_RTC  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x01;				// OSD RTC control

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Get_RTC ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_OSD_Get_RTC ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	for(i=0; i<7; i++)
		printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	printf("XU_OSD_Get_RTC <== Success \n");
	
	*year = (xctrl.data[5]<<8) | xctrl.data[6];
	*month = xctrl.data[4];
	*day = xctrl.data[3];
	*hour = xctrl.data[2];
	*minute = xctrl.data[1];
	*second = xctrl.data[0];

	printf(" year 	= %d \n",*year);
	printf(" month	= %d \n",*month);
	printf(" day 	= %d \n",*day);
	printf(" hour 	= %d \n",*hour);
	printf(" minute	= %d \n",*minute);
	printf(" second	= %d \n",*second);
	
	return 0;
}

int XU_OSD_Set_Size(int fd, unsigned char LineSize, unsigned char BlockSize)
{	
	printf("XU_OSD_Set_Size  ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;				// OSD Size control

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Size ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	if (LineSize > 4)
		LineSize = 4;

	if (BlockSize > 4)
		BlockSize = 4;
		
	// Set data
	xctrl.data[0] = LineSize;
	xctrl.data[1] = BlockSize;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Size ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_OSD_Set_Size <== Success \n");

	return 0;
}

int XU_OSD_Get_Size(int fd, unsigned char *LineSize, unsigned char *BlockSize)
{	
	printf("XU_OSD_Get_Size  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;				// OSD Size control

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Get_Size ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_OSD_Get_Size ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	for(i=0; i<2; i++)
		printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*LineSize = xctrl.data[0];
	*BlockSize = xctrl.data[1];

	printf("OSD Size (Line) = %d\n",*LineSize);
	printf("OSD Size (Block) = %d\n",*BlockSize);
	printf("XU_OSD_Get_Size <== Success \n");
	
	return 0;
}

int XU_OSD_Set_Color(int fd, unsigned char FontColor, unsigned char BorderColor)
{	
	printf("XU_OSD_Set_Color  ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// OSD Color control

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Color ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	if (FontColor > 4)
		FontColor = 4;

	if (BorderColor > 4)
		BorderColor = 4;
		
	// Set data
	xctrl.data[0] = FontColor;
	xctrl.data[1] = BorderColor;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Color ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_OSD_Set_Color <== Success \n");

	return 0;
}

int XU_OSD_Get_Color(int fd, unsigned char *FontColor, unsigned char *BorderColor)
{	
	printf("XU_OSD_Get_Color  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// OSD Color control

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Get_Color ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_OSD_Get_Color ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	for(i=0; i<2; i++)
		printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*FontColor = xctrl.data[0];
	*BorderColor = xctrl.data[1];

	printf("OSD Font Color = %d\n",*FontColor );
	printf("OSD Border Color = %d\n",*BorderColor);
	printf("XU_OSD_Get_Color <== Success \n");
	
	return 0;
}

int XU_OSD_Set_Enable(int fd, unsigned char Enable_Line, unsigned char Enable_Block)
{	
	printf("XU_OSD_Set_Enable  ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x04;				// OSD enable

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Enable ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
		
	// Set data
	xctrl.data[0] = Enable_Line;
	xctrl.data[1] = Enable_Block;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Enable ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_OSD_Set_Enable <== Success \n");

	return 0;
}

int XU_OSD_Get_Enable(int fd, unsigned char *Enable_Line, unsigned char *Enable_Block)
{	
	printf("XU_OSD_Get_Enable  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x04;				// OSD Enable

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Get_Enable ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_OSD_Get_Enable ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	for(i=0; i<2; i++)
		printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*Enable_Line = xctrl.data[0];
	*Enable_Block = xctrl.data[1];
	
	printf("OSD Enable Line = %d\n",*Enable_Line);
	printf("OSD Enable Block = %d\n",*Enable_Block);

	printf("XU_OSD_Get_Enable <== Success \n");
	
	return 0;
}

int XU_OSD_Set_AutoScale(int fd, unsigned char Enable_Line, unsigned char Enable_Block)
{	
	printf("XU_OSD_Set_AutoScale  ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x05;				// OSD Auto Scale enable

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_AutoScale ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
		
	// Set data
	xctrl.data[0] = Enable_Line;
	xctrl.data[1] = Enable_Block;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_AutoScale ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_OSD_Set_AutoScale <== Success \n");

	return 0;
}

int XU_OSD_Get_AutoScale(int fd, unsigned char *Enable_Line, unsigned char *Enable_Block)
{	
	printf("XU_OSD_Get_AutoScale  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x05;				// OSD Auto Scale enable

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Get_AutoScale ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_OSD_Get_AutoScale ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	for(i=0; i<2; i++)
		printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*Enable_Line = xctrl.data[0];
	*Enable_Block = xctrl.data[1];

	printf("OSD Enable Line  Auto Scale = %d\n",*Enable_Line);
	printf("OSD Enable Block Auto Scale = %d\n",*Enable_Block);
	printf("XU_OSD_Get_AutoScale <== Success \n");
	
	return 0;
}

int XU_OSD_Set_Multi_Size(int fd, unsigned char Stream0, unsigned char Stream1, unsigned char Stream2)
{	
	printf("XU_OSD_Set_Multi_Size  %d   %d   %d  ==>\n",Stream0 ,Stream1 , Stream2);

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x06;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Multi_Size ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// Set data
	xctrl.data[0] = Stream0;
	xctrl.data[1] = Stream1;
	xctrl.data[2] = Stream2;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Multi_Size ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_OSD_Set_Multi_Size <== Success \n");

	return 0;
}

int XU_OSD_Get_Multi_Size(int fd, unsigned char *Stream0, unsigned char *Stream1, unsigned char *Stream2)
{	
	printf("XU_OSD_Get_Multi_Size  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x06;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Get_Multi_Size ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_OSD_Get_Multi_Size ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	for(i=0; i<3; i++)
		printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*Stream0 = xctrl.data[0];
	*Stream1 = xctrl.data[1];
	*Stream2 = xctrl.data[2];
	
	printf("OSD Multi Stream 0 Size = %d\n",*Stream0);
	printf("OSD Multi Stream 1 Size = %d\n",*Stream1);
	printf("OSD Multi Stream 2 Size = %d\n",*Stream2);
	printf("XU_OSD_Get_Multi_Size <== Success \n");
	
	return 0;
}

int XU_OSD_Set_Start_Position(int fd, unsigned char OSD_Type, unsigned int RowStart, unsigned int ColStart)
{	
	printf("XU_OSD_Set_Start_Position  ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x08;				// OSD Start Position

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Start_Position ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	if (OSD_Type > 3)
		OSD_Type = 0;
		
	// Set data
	xctrl.data[0] = OSD_Type;
	xctrl.data[1] = (RowStart & 0xFF00) >> 8;	//unit 16 lines
	xctrl.data[2] = RowStart & 0x00FF;
	xctrl.data[3] = (ColStart & 0xFF00) >> 8;	//unit 16 pixels
	xctrl.data[4] = ColStart & 0x00FF;	

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Start_Position ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_OSD_Set_Start_Position <== Success \n");

	return 0;
}

int XU_OSD_Get_Start_Position(int fd, unsigned int *LineRowStart, unsigned int *LineColStart, unsigned int *BlockRowStart, unsigned int *BlockColStart)
{	
	printf("XU_OSD_Set_Start_Position  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x08;				// OSD Start Position

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Start_Position ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_OSD_Set_Start_Position ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	for(i=0; i<8; i++)
		printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*LineRowStart = (xctrl.data[0] << 8) | xctrl.data[1];
	*LineColStart = (xctrl.data[2] << 8) | xctrl.data[3];
	*BlockRowStart = (xctrl.data[4] << 8) | xctrl.data[5];
	*BlockColStart = (xctrl.data[6] << 8) | xctrl.data[7];
	
	printf("OSD Line Start Row =%d * 16lines\n",*LineRowStart);
	printf("OSD Line Start Col =%d * 16pixels\n",*LineColStart);
	printf("OSD Block Start Row =%d * 16lines\n",*BlockRowStart);
	printf("OSD Block Start Col =%d * 16pixels\n",*BlockColStart);
	printf("XU_OSD_Set_Start_Position <== Success \n");
	
	return 0;
}

int XU_OSD_Set_String(int fd, char *String)
{	
	printf("XU_OSD_Set_String  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x07;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_String ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// Set data
	for(i=0; i<11; i++)
		xctrl.data[i] = String[i];
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Set_String ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_OSD_Set_String <== Success \n");

	return 0;
}

int XU_OSD_Get_String(int fd, char *String)
{	
	printf("XU_OSD_Get_String  ==>\n");

	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_OSD_CTRL,				/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};
	
	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x07;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_OSD_Get_String ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_OSD_Get_String ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	for(i=0; i<11; i++)
	{
		String[i] = xctrl.data[i];
		printf("      Get data[%d] : 0x%x\n", i, String[i]);
	}

	printf("OSD String = %s \n",String);
	printf("XU_OSD_Get_String <== Success \n");

	return 0;
}

int XU_MD_Set_Mode(int fd, unsigned char Enable)
{	
	printf("XU_MD_Set_Mode  ==>\n");

	int ret = 0;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MOTION_DETECTION,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x01;				// Motion detection mode

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Set_Mode ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set data
	xctrl.data[0] = Enable;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Set_Mode ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_MD_Set_Mode <== Success \n");

	return 0;
}

int XU_MD_Get_Mode(int fd, unsigned char *Enable)
{	
	printf("XU_MD_Get_Mode  ==>\n");

	int ret = 0;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MOTION_DETECTION,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x01;				// Motion detection mode

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Get_Mode ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_MD_Get_Mode ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	*Enable = xctrl.data[0];

	printf("Motion Detect mode = %d\n",*Enable);
	printf("XU_MD_Get_Mode <== Success \n");
	
	return 0;
}

int XU_MD_Set_Threshold(int fd, unsigned int MD_Threshold)
{	
	printf("XU_MD_Set_Threshold  ==>\n");

	int ret = 0;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MOTION_DETECTION,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;				// Motion detection threshold

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Set_Threshold ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set data
	xctrl.data[0] = (MD_Threshold & 0xFF00) >> 8;
	xctrl.data[1] = MD_Threshold & 0x00FF;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Set_Threshold ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_MD_Set_Threshold <== Success \n");

	return 0;
}

int XU_MD_Get_Threshold(int fd, unsigned int *MD_Threshold)
{	
	printf("XU_MD_Get_Threshold  ==>\n");

	int ret = 0;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MOTION_DETECTION,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;				// Motion detection threshold

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Get_Threshold ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_MD_Get_Threshold ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}

	*MD_Threshold = (xctrl.data[0] << 8) | xctrl.data[1];
	
	printf("Motion Detect threshold = %d\n",*MD_Threshold);
	printf("XU_MD_Get_Threshold <== Success \n");
	
	return 0;
}

int XU_MD_Set_Mask(int fd, unsigned char *Mask)
{
	int ret = 0;
	unsigned char i;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MOTION_DETECTION,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// Motion detection mask

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Set_Mask ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set data
	for(i=0; i < 24; i++) {
		xctrl.data[i] = Mask[i];
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Set_Mask ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	return 0;
}

int XU_MD_Get_Mask(int fd, unsigned char *Mask)
{
	int ret = 0;
	int i,j,k;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MOTION_DETECTION,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// Motion detection mask

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Get_Mask ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)  \n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_MD_Get_Mask ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i) \n", ret);
		return -1;
	}

	for(i=0; i<24; i++) {
		printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);
		Mask[i] = xctrl.data[i];
	}
	
	printf("               ======   Motion Detect Mask   ======                \n");
	printf("     1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 \n");
	
	for(k=0; k<12; k++) {
		printf("%2d   ",k+1);
		for(j=0; j<2; j++) {
			for(i=0; i<8; i++)
				printf("%d   ",(Mask[k*2+j]>>i)&0x01);
		}
		printf("\n");
	}
	
	return 0;
}

int XU_MD_Set_RESULT(int fd, unsigned char *Result)
{
	int ret = 0;
	unsigned char i;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MOTION_DETECTION,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x04;				// Motion detection Result

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Set_RESULT ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set data
	for(i=0; i < 24; i++)
	{
		xctrl.data[i] = Result[i];
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Set_RESULT ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	return 0;
}

int XU_MD_Get_RESULT(int fd, unsigned char *Result)
{
	int ret = 0;
	int i,j,k;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		.unit = XU_SONIX_USR_ID,						/* bUnitID				*/
		.selector = XU_SONIX_USR_MOTION_DETECTION,		/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data 				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x04;				// Motion detection Result

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MD_Get_RESULT ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)  \n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_MD_Get_RESULT ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i) \n", ret);
		return -1;
	}

	for(i=0; i<24; i++)
	{
		//printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);
		Result[i] = xctrl.data[i];
	}

	system("clear");
	printf("               ------   Motion Detect Result   ------                \n");
	printf("     1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 \n");
	
	for(k=0; k<12; k++) {
		printf("%2d   ",k+1);
		for(j=0; j<2; j++) {
			for(i=0; i<8; i++)
				printf("%d   ",(Result[k*2+j]>>i)&0x01);
		}
		printf("\n");
	}
	
	return 0;
}

int XU_MJPG_Get_Bitrate(int fd, unsigned int *MJPG_Bitrate)
{
	printf("XU_MJPG_Get_Bitrate ==>\n");
	int i = 0;
	int ret = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;
	*MJPG_Bitrate = 0;

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_MJPG_CTRL;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_MJPG_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MJPG_Get_Bitrate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	// Get Bit rate ctrl number
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_MJPG_Get_Bitrate ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}
	
	printf("   == XU_MJPG_Get_Bitrate Success == \n");

	if (chip_id == CHIP_SNC291A) {
		for(i=0; i<2; i++)
			printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

		BitRate_CtrlNum = ( xctrl.data[0]<<8 )| (xctrl.data[1]) ;

		// Bit Rate = BitRate_Ctrl_Num*256*fps*8 /1024(Kbps)
		*MJPG_Bitrate = (BitRate_CtrlNum*256.0*m_CurrentFPS*8)/1024.0;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		for(i=0; i<4; i++)
			printf("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

		*MJPG_Bitrate = (xctrl.data[0] << 24) | (xctrl.data[1] << 16) | (xctrl.data[2] << 8) | (xctrl.data[3]) ;
	}
	
	printf("XU_MJPG_Get_Bitrate (%x)<==\n", *MJPG_Bitrate);
	return 0;
}

int XU_MJPG_Set_Bitrate(int fd, unsigned int MJPG_Bitrate)
{	
	printf("XU_MJPG_Set_Bitrate (%x) ==>\n",MJPG_Bitrate);
	int ret = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_MJPG_CTRL;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_MJPG_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	}
	
	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MJPG_Set_Bitrate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set Bit Rate Ctrl Number
	if (chip_id == CHIP_SNC291A) {
		// Bit Rate = BitRate_Ctrl_Num*256*fps*8/1024 (Kbps)
		BitRate_CtrlNum = ((MJPG_Bitrate*1024)/(256*m_CurrentFPS*8));

		xctrl.data[0] = (BitRate_CtrlNum & 0xFF00) >> 8;
		xctrl.data[1] = (BitRate_CtrlNum & 0x00FF);
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.data[0] = (MJPG_Bitrate & 0xFF000000) >> 24;
		xctrl.data[1] = (MJPG_Bitrate & 0x00FF0000) >> 16;
		xctrl.data[2] = (MJPG_Bitrate & 0x0000FF00) >> 8;
		xctrl.data[3] = (MJPG_Bitrate & 0x000000FF);
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_MJPG_Set_Bitrate ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}
	
	printf("XU_MJPG_Set_Bitrate <== Success \n");
	return 0;
}

int XU_IMG_Set_Mirror(int fd, unsigned char Mirror)
{	
	printf("XU_IMG_Set_Mirror  ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_IMG_Set_Mirror ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set data
	xctrl.data[0] = Mirror;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_IMG_Set_Mirror ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_IMG_Set_Mirror  0x%x <== Success \n",Mirror);

	return 0;
}

int XU_IMG_Get_Mirror(int fd, unsigned char *Mirror)
{	
	printf("XU_IMG_Get_Mirror  ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_IMG_Get_Mirror ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_IMG_Get_Mirror ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}
	
	*Mirror = xctrl.data[0];

	printf("Mirror = %d\n",*Mirror);
	printf("XU_IMG_Get_Mirror <== Success \n");
	
	return 0;
}

int XU_IMG_Set_Flip(int fd, unsigned char Flip)
{	
	printf("XU_IMG_Set_Flip  ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_IMG_Set_Flip ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set data
	xctrl.data[0] = Flip;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_IMG_Set_Flip ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_IMG_Set_Flip  0x%x <== Success \n",Flip);

	return 0;
}

int XU_IMG_Get_Flip(int fd, unsigned char *Flip)
{	
	printf("XU_IMG_Get_Flip  ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_IMG_Get_Flip ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_IMG_Get_Flip ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}
	
	*Flip = xctrl.data[0];

	printf("Flip = %d\n",*Flip);
	printf("XU_IMG_Get_Flip <== Success \n");
	
	return 0;
}

int XU_IMG_Set_Color(int fd, unsigned char Color)
{	
	printf("XU_IMG_Set_Color  ==>\n");

	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_IMG_Set_Color ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Set data
	xctrl.data[0] = Color;

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_IMG_Set_Color ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	printf("XU_IMG_Set_Color  0x%x <== Success \n",Color);

	return 0;
}

int XU_IMG_Get_Color(int fd, unsigned char *Color)
{
	int ret = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		.unit = 0,										/* bUnitID 				*/
		.selector = 0,									/* function selector	*/
		.size = sizeof(ctrldata),						/* data size			*/
		.data = ctrldata								/* data					*/
	};

	if (chip_id == CHIP_SNC291A) {
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;
	} else if ((chip_id == CHIP_SNC291B) || (chip_id == CHIP_SNC292A)) {
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;
	}

	if ((ret = ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) {
		printf("XU_IMG_Get_Color ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n", ret);
		return -1;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((ret = ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) {
		printf("XU_IMG_Get_Color ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n", ret);
		return -1;
	}
	
	*Color = xctrl.data[0];

	printf("Image Color = %d\n",*Color);
	printf("XU_IMG_Get_Color <== Success \n");
	
	return 0;
}
