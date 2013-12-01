#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void SaveFrame(AVFrame *frame, int width, int height, int iFrame)
{
	FILE *pFile;
	char filename[32];
	int y;

	sprintf(filename, "%s%d.ppm", "frame", iFrame);
	pFile = fopen(filename, "wb+");
	if (pFile == NULL)
		return;

	fprintf(pFile, "P6\n%d %d\n255\n", width, height);
	for (y = 0; y < height; y++) {
		fwrite(frame->data[0] + y*frame->linesize[0], 1, width*3, pFile);
	}

	fclose(pFile);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("%s [video_file]\n", argv[0]);
		exit(-1);
	}

	char *filename = argv[1];
	int ret = 0;

	// register codec and others
    avcodec_register_all();
    av_register_all();

	// setup format context from file
	static AVFormatContext *pFormatCtx = NULL;
	if ((ret = avformat_open_input(&pFormatCtx, filename, NULL, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "cannot open input file\n");
		exit(-1);
	}

	// find stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("unable to find stream info in file\n");
		exit(-1);
	}

	// dump format
	av_dump_format(pFormatCtx, 0, filename, 0);

	// find codec context for video stream
	AVCodecContext *pCodecCtx = NULL;
	int videoStream = -1;
	int i = 0;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	if (videoStream == -1) {
		printf("video stream not found\n");
		exit(-1);
	}
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;
	
	// find decoder base on video codec
	AVCodec *pCodec = NULL;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		printf("failed to find a decoder\n");
		exit(-1);
	}

	// open decoder
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("failed to open codec\n");
		exit(-1);
	}

	// alloc video frame
	AVFrame *pFrame = NULL;
	AVFrame *pFrameRGB = NULL;	
	pFrame = avcodec_alloc_frame();
	pFrameRGB = avcodec_alloc_frame();
	if (pFrame == NULL || pFrameRGB == NULL) {
		printf("failed to alloc frame\n");
		exit(-1);
	}

	// alloc RGP24 Frame buffer
	uint8_t *buffer = NULL;
	int numBytes;
	numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
	buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	if (buffer == NULL) {
		printf("failed to alloc buffer\n");
		exit(-1);
	}

	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

	int frameFinished;
	AVPacket packet;

	// setup sws context;
	struct SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

	i = 0;
	while(av_read_frame(pFormatCtx, &packet) >= 0) {
		printf("read frame: \n");
		printf("    packet.stream_index : %d\n", packet.stream_index);
		printf("	packet.size         : %d\n", packet.size);
		printf("    frame finished      : %d\n", frameFinished);
		if (packet.stream_index == videoStream) {
			// decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			if (frameFinished) {
				printf("     pix_fmt              : %d\n", pCodecCtx->pix_fmt);
				printf("     width                : %d\n", pCodecCtx->width);
				printf("     height                : %d\n", pCodecCtx->height);
				// convert image
				sws_scale(img_convert_ctx, (const uint8_t * const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
				
				if (++i <= 10) {
					SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
				}
			}
		}
		av_free_packet(&packet);
	}

	sws_freeContext(img_convert_ctx);
	
	av_free(buffer);
	av_free(pFrameRGB);
	av_free(pFrame);

	avcodec_close(pCodecCtx);

	avformat_close_input(&pFormatCtx);

	return 0;
}
