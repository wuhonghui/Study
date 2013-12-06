#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>

#include <SDL.h>
#include <SDL_thread.h>

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("%s [video_name]\n", argv[0]);
		return -1;
	}
	
	int ret = 0;
	int i = 0;
	char *filename = argv[1];

	avcodec_register_all();
	av_register_all();

	// 打开文件，检测格式和媒体流
	AVFormatContext *format_ctx = NULL;
	if (avformat_open_input(&format_ctx, filename, NULL, NULL) < 0) {
		av_log(NULL, AV_LOG_ERROR, "failed to open file %s", filename);
		return -1;
	}
	if (avformat_find_stream_info(format_ctx, NULL) < 0) {
		av_log(NULL, AV_LOG_ERROR, "failed to find stream info");
		return -1;
	}

	// 寻找视频、音频流
	AVCodecContext *video_codec_ctx = NULL;
	AVCodecContext *audio_codec_ctx = NULL;
	AVCodec *video_codec = NULL;
	AVCodec *audio_codec = NULL;
	int video_stream_idx = -1;
	int audio_stream_idx = -1;
	for (i = 0; i < format_ctx->nb_streams; i++) {
		if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_codec_ctx = format_ctx->streams[i]->codec;
			video_stream_idx = i;
		} else if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_codec_ctx = format_ctx->streams[i]->codec;
			audio_stream_idx = i;
		}
	}

	// 寻找视频、音频流的解码器
	if (video_codec_ctx) {
		video_codec = avcodec_find_decoder(video_codec_ctx->codec_id);
		if (video_codec == NULL) {
			av_log(NULL, AV_LOG_ERROR, "failed to find video decoder");
			return -1;
		}
		if (avcodec_open2(video_codec_ctx, video_codec, NULL) < 0) {
			av_log(NULL, AV_LOG_ERROR, "failed to open codec");
			return -1;
		}
	} else {
		av_log(NULL, AV_LOG_ERROR, "video stream not found");
	}
	if (audio_codec_ctx) {
		audio_codec = avcodec_find_decoder(audio_codec_ctx->codec_id);
		if (audio_codec == NULL) {
			av_log(NULL, AV_LOG_ERROR, "failed to find audio decoder");
			return -1;
		}
		if (avcodec_open2(audio_codec_ctx, audio_codec, NULL) < 0) {
			av_log(NULL, AV_LOG_ERROR, "faild to open codec");
			return -1;
		}
	} else {
		av_log(NULL, AV_LOG_ERROR, "audio stream not found\n");
	}

	// 分配帧缓冲区
	AVFrame *video_frame = NULL;
	AVFrame *video_frame_rgb = NULL;
	video_frame = avcodec_alloc_frame();
	video_frame_rgb = avcodec_alloc_frame();
	if (video_frame == NULL || video_frame_rgb == NULL) {
		av_log(NULL, AV_LOG_ERROR, "failed to alloc video frame\n");
		return -1;
	}
	
	uint8_t *buffer = NULL;
	int num_bytes = 0;
	num_bytes = avpicture_get_size(PIX_FMT_RGB32, video_codec_ctx->width, video_codec_ctx->height);
	buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
	if (!buffer) {
		av_log(NULL, AV_LOG_ERROR, "failed to alloc RGB video buffer\n");
		return -1;
	}
	avpicture_fill((AVPicture *)video_frame_rgb, buffer, PIX_FMT_RGB32, 
				video_codec_ctx->width, video_codec_ctx->height);
	
	// 格式转换上下文
	struct SwsContext *img_convert_ctx = NULL;
	img_convert_ctx = sws_getContext(video_codec_ctx->width, video_codec_ctx->height, video_codec_ctx->pix_fmt, 
								video_codec_ctx->width, video_codec_ctx->height, PIX_FMT_RGB32,
								SWS_BICUBIC, NULL, NULL, NULL);

	int frame_finished = 0;
	AVPacket packet;

	// 初始化 Simple Direct Layer
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)){
		fprintf(stderr, "failed to init SDL - %s\n", SDL_GetError());
		exit(-1);
	}
	
	// 创建显示窗口
	SDL_Window *sdl_window = SDL_CreateWindow("My Video Window", 
										SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
										video_codec_ctx->width, video_codec_ctx->height,
										0);
	if (!sdl_window) {
		fprintf(stderr, "failed to create screen\n");
		exit(-1);
	}

	// 创建渲染上下文
	SDL_Renderer *sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);
	if (!sdl_renderer) {
		fprintf(stderr, "failed to create renderer\n");
		exit(-1);
	}
	SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
	SDL_RenderClear(sdl_renderer);
	SDL_RenderPresent(sdl_renderer);

	// 创建纹理图案
	SDL_Texture *sdl_texture = SDL_CreateTexture(sdl_renderer, 
											SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
											video_codec_ctx->width, video_codec_ctx->height);
	if (!sdl_texture) {
		fprintf(stderr, "failed to create texture\n");
		exit(-1);
	}


	// 从文件中读取packet，送入解码器解码
	i = 0;
	while (av_read_frame(format_ctx, &packet) >= 0) {
		if (packet.stream_index == video_stream_idx) {
			// 取得一packet的图像数据
			ret = avcodec_decode_video2(video_codec_ctx, video_frame, &frame_finished, &packet);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "error decode video\n");
				break;
			}

			if (frame_finished) {
				// 取得一帧完整图像, 做处理
				sws_scale(img_convert_ctx, (const uint8_t * const*)video_frame->data, video_frame->linesize, 
						0, video_codec_ctx->height, 
						video_frame_rgb->data, video_frame_rgb->linesize);

				// 显示RGB图像
				SDL_UpdateTexture(sdl_texture, NULL, buffer, video_frame_rgb->linesize[0]);
				SDL_RenderClear(sdl_renderer);
				SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
				SDL_RenderPresent(sdl_renderer);

				// 间隔30ms以产生正常播放速度
				usleep(30*1000);
			}
		} else if (packet.stream_index == audio_stream_idx) {
			// 取得一packet的音频数据
			;
		} else {
			// 其他数据
			;
		}
		av_free_packet(&packet);
	}

quit:
	if (sdl_texture)
		SDL_DestroyTexture(sdl_texture);
	if (sdl_renderer)
		SDL_DestroyRenderer(sdl_renderer);
	if (sdl_window)
		SDL_DestroyWindow(sdl_window);
	if (img_convert_ctx)
		sws_freeContext(img_convert_ctx);

	if (buffer)
		av_free(buffer);
	if (video_frame_rgb)
		av_free(video_frame_rgb);
	if (video_frame)
		av_free(video_frame);

	if (video_codec_ctx)
		avcodec_close(video_codec_ctx);
	if (audio_codec_ctx)
		avcodec_close(audio_codec_ctx);

	if (format_ctx)
		avformat_close_input(&format_ctx);

	return 0;
}
