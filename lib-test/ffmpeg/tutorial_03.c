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

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

struct packet_queue{
    AVPacketList *first, *last;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
    int abort;
};

void packet_queue_init(struct packet_queue *queue)
{
    memset(queue, 0, sizeof(struct packet_queue));
    queue->mutex = SDL_CreateMutex();
    queue->cond = SDL_CreateCond();
}

int packet_queue_put(struct packet_queue *queue, AVPacket *packet)
{
    if (!queue)
        return -1;
    if (!packet)
        return 0;

    AVPacketList *packet_list = NULL;
    packet_list = av_malloc(sizeof(AVPacketList));
    if (!packet_list)
        return -1;
    packet_list->next = NULL;
    packet_list->pkt = *packet;

    SDL_LockMutex(queue->mutex);

    if (queue->last == NULL)
        queue->first = packet_list;
    else
        queue->last->next = packet_list;
    queue->last = packet_list;
    queue->nb_packets++;
    queue->size += packet_list->pkt.size;

    SDL_CondSignal(queue->cond);

    SDL_UnlockMutex(queue->mutex);

    return 0;
}

int packet_queue_get(struct packet_queue *queue, AVPacket *packet, int wait)
{
    AVPacketList *packet_list = NULL;
    int ret = 0;
    
    SDL_LockMutex(queue->mutex);

    for (;;) {
        if (queue->abort) {
            ret = -1;
            break;
        }
        packet_list = queue->first;
        if (packet_list == NULL) {
            if (wait)
                SDL_CondWait(queue->cond, queue->mutex);
            else {
                ret = -1;
                break;
            }
        } else {
            queue->first = packet_list->next;
            if (queue->first == NULL)
                queue->last = NULL;
            queue->nb_packets--;
            queue->size -= packet_list->pkt.size;
            *packet = packet_list->pkt;

            av_free(packet_list);
            ret = 1;
            break;
        }
    }

    SDL_UnlockMutex(queue->mutex);
    
    return ret;
}

struct packet_queue audio_pkt_queue;

int audio_decode_frame(AVCodecContext *avctx, uint8_t *buf, int buf_size)
{
    static AVPacket pkt = {0};
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;
    static AVFrame frame = {{0}};

    int len1, data_size = 0;

    for (;;) {
        while (audio_pkt_size > 0) {
            int got_frame = 0;
            len1 = avcodec_decode_audio4(avctx, &frame, &got_frame, &pkt);
            if (len1 < 0) {
                /* error decode, skip frame*/
                audio_pkt_size = 0;
                break;
            }
            audio_pkt_size -= len1;
            audio_pkt_data += len1;
            if (got_frame) {
                data_size = av_samples_get_buffer_size(
                                NULL,
                                avctx->channels,
                                frame.nb_samples,
                                avctx->sample_fmt,
                                1
                            );
                memcpy(buf, frame.data[0], data_size);
            }
            if (data_size <= 0)
                continue;
            return data_size;
        }

        if (pkt.data) {
            av_free_packet(&pkt);
        }
        
        if (packet_queue_get(&audio_pkt_queue, &pkt, 1) < 0)
            return -1;
            
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
    }
}

void audio_callback(void *userdata, uint8_t * stream, int len)
{
    AVCodecContext *codec_ctx = userdata;
    int data_len = 0;
    int audio_size = 0;

    static uint8_t audio_buf[MAX_AUDIO_FRAME_SIZE * 3 / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;

    while(len > 0)
    {
        if(audio_buf_index >= audio_buf_size)
        {
            /* We have already sent all our data; get more */
            audio_size = audio_decode_frame(codec_ctx, audio_buf, audio_buf_size);
            if(audio_size < 0)
            {
                /* If error, output silence */
                audio_buf_size = 1024;
                memset(audio_buf, 0, audio_buf_size);
            }
            else
            {
                audio_buf_size = audio_size;
            }
            audio_buf_index = 0;
        }
        data_len = audio_buf_size - audio_buf_index;
        if(data_len > len)
            data_len = len;
        memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, data_len);
        len -= data_len;
        stream += data_len;
        audio_buf_index += data_len;
    }
}


int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("%s [video_name]\n", argv[0]);
		return -1;
	}
	
	int ret = 0;
	int i = 0;
	char *filename = argv[1];

    av_log_set_level(AV_LOG_DEBUG);
	avcodec_register_all();
	av_register_all();

	// 打开文件，检测格式和媒体流
	AVFormatContext *format_ctx = NULL;
	if (avformat_open_input(&format_ctx, filename, NULL, NULL) < 0) {
		av_log(NULL, AV_LOG_ERROR, "failed to open file %s\n", filename);
		return -1;
	}
	if (avformat_find_stream_info(format_ctx, NULL) < 0) {
		av_log(NULL, AV_LOG_ERROR, "failed to find stream info\n");
		return -1;
	}

    av_dump_format(format_ctx, 0, filename, 0);

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
			av_log(NULL, AV_LOG_ERROR, "failed to find video decoder\n");
			return -1;
		}
		if (avcodec_open2(video_codec_ctx, video_codec, NULL) < 0) {
			av_log(NULL, AV_LOG_ERROR, "failed to open codec\n");
			return -1;
		}
	} else {
		av_log(NULL, AV_LOG_ERROR, "video stream not found\n");
	}
	if (audio_codec_ctx) {
		audio_codec = avcodec_find_decoder(audio_codec_ctx->codec_id);
		if (audio_codec == NULL) {
			av_log(NULL, AV_LOG_ERROR, "failed to find audio decoder\n");
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
	AVPacket packet;
    int frame_finished = 0;

    uint8_t *buffer = NULL;
    int num_bytes = 0;
    
    struct SwsContext *img_convert_ctx = NULL;
    SDL_Renderer *sdl_renderer = NULL;
    SDL_Window *sdl_window = NULL;
    SDL_Texture *sdl_texture = NULL;
    
	video_frame = avcodec_alloc_frame();
	video_frame_rgb = avcodec_alloc_frame();
	if (video_frame == NULL || video_frame_rgb == NULL) {
		av_log(NULL, AV_LOG_ERROR, "failed to alloc video frame\n");
		return -1;
	}

	if (video_codec_ctx) {

    	num_bytes = avpicture_get_size(PIX_FMT_RGB32, video_codec_ctx->width, video_codec_ctx->height);
    	buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
    	if (!buffer) {
    		av_log(NULL, AV_LOG_ERROR, "failed to alloc RGB video buffer\n");
    		return -1;
    	}
    	avpicture_fill((AVPicture *)video_frame_rgb, buffer, PIX_FMT_RGB32, 
    				video_codec_ctx->width, video_codec_ctx->height);
    	
    	// 格式转换上下文
    	img_convert_ctx = sws_getContext(video_codec_ctx->width, video_codec_ctx->height, video_codec_ctx->pix_fmt, 
        								video_codec_ctx->width, video_codec_ctx->height, PIX_FMT_RGB32,
        								SWS_BICUBIC, NULL, NULL, NULL);

    	// 初始化 Simple Direct Layer
    	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)){
    		fprintf(stderr, "failed to init SDL - %s\n", SDL_GetError());
    		exit(-1);
    	}
    	
    	// 创建显示窗口
    	sdl_window = SDL_CreateWindow("My Video Window", 
     									SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
     									video_codec_ctx->width, video_codec_ctx->height,
     									0);
    	if (!sdl_window) {
    		fprintf(stderr, "failed to create screen\n");
    		exit(-1);
    	}

    	// 创建渲染上下文
    	sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);
    	if (!sdl_renderer) {
    		fprintf(stderr, "failed to create renderer\n");
    		exit(-1);
    	}
    	SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
    	SDL_RenderClear(sdl_renderer);
    	SDL_RenderPresent(sdl_renderer);

    	// 创建纹理图案
    	sdl_texture = SDL_CreateTexture(sdl_renderer, 
										SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
										video_codec_ctx->width, video_codec_ctx->height);
    	if (!sdl_texture) {
    		fprintf(stderr, "failed to create texture\n");
    		exit(-1);
    	}
	}

    if (audio_codec_ctx) {
        SDL_AudioSpec wanted_audio_spec;
        SDL_AudioSpec audio_spec;
        wanted_audio_spec.freq = audio_codec_ctx->sample_rate;
        wanted_audio_spec.format = AUDIO_S16SYS;
        wanted_audio_spec.channels = audio_codec_ctx->channels;
        wanted_audio_spec.silence = 0;
        wanted_audio_spec.samples = 1024;
        wanted_audio_spec.callback = audio_callback;
        wanted_audio_spec.userdata = audio_codec_ctx;
        if (SDL_OpenAudio(&wanted_audio_spec, &audio_spec)) {
            fprintf(stderr, "failed to open audio\n");
            exit(-1);
        }

        packet_queue_init(&audio_pkt_queue);
        SDL_PauseAudio(0);
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
			av_free_packet(&packet);
		} else if (packet.stream_index == audio_stream_idx) {
			// 取得一packet的音频数据
			packet_queue_put(&audio_pkt_queue, &packet);
		} else {
			// 其他数据
			av_free_packet(&packet);
		}

        SDL_Event ev;
        SDL_PollEvent(&ev);
        switch(ev.type) {
        case SDL_QUIT:
            SDL_Quit();
            audio_pkt_queue.abort = 1;
            goto quit;
        default:
            break;
        }
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
