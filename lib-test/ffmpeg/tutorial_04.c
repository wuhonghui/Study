#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/avstring.h>
#include <libswscale/swscale.h>

#include <SDL.h>
#include <SDL_thread.h>

#define VIDEO_PICTURE_QUEUE_SIZE 2

#define SDL_AUDIO_BUFFER_SIZE   1024
#define MAX_AUDIO_FRAME_SIZE    192000

#define MAX_AUDIOQ_SIZE     (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE     (5 * 256 * 1024)

#define FF_ALLOC_EVENT      (SDL_USEREVENT)
#define FF_REFRESH_EVENT    (SDL_USEREVENT + 1)
#define FF_QUIT_EVENT       (SDL_USEREVENT + 2)

#define DEBUG() av_log(NULL, AV_LOG_DEBUG, "%s: %d\n", __func__, __LINE__)

typedef struct VideoPictureQueue{
    SDL_Texture *picture[VIDEO_PICTURE_QUEUE_SIZE];
    int nb_pictures;
    int rindex, windex;
    SDL_mutex *mutex;
    SDL_cond *cond;
}VideoPictureQueue;

typedef struct AVPacketQueue{
    AVPacketList *first, *last;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
}AVPacketQueue;

typedef struct AudioPacketDecodeCtx{
    uint8_t audio_buf[MAX_AUDIO_FRAME_SIZE];
    unsigned int audio_buf_size;
    unsigned int audio_buf_index;

    AVPacket audio_pkt;
    AVFrame audio_frame;
}AudioPacketDecodeCtx;

typedef struct VideoState {
    AVFormatContext *fmt_ctx;

    int video_stream;
    int audio_stream;
    
    AVStream *audio_st;
    AVStream *video_st;

    struct SwsContext *sws_ctx;
    AVPacketQueue video_queue;
    VideoPictureQueue video_picture_queue;
    
    AVPacketQueue audio_queue;
    AudioPacketDecodeCtx audio_decode_ctx;
    
    SDL_Thread *video_tid;
    SDL_Thread *decode_tid;

    char filename[1024];
    int quit;
}VideoState;

typedef struct SDLContext{
    SDL_Window *window;

    SDL_Renderer *renderer;

    SDL_Texture *texture;
    int width, height;
    char *title;
}SDLContext;

/* global variable */
VideoState *g_video_state = NULL;
SDLContext *g_sdlctx = NULL;

void avpacket_queue_init(AVPacketQueue *queue)
{
    memset(queue, 0, sizeof(AVPacketQueue));
    queue->mutex = SDL_CreateMutex();
    queue->cond = SDL_CreateCond();
}

int avpacket_queue_put(AVPacketQueue *queue, AVPacket *packet)
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

int avpacket_queue_get(AVPacketQueue *queue, AVPacket *packet, int wait)
{
    AVPacketList *packet_list = NULL;
    int ret = 0;
    
    SDL_LockMutex(queue->mutex);

    for (;;) {
        if (g_video_state->quit) {
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

void video_picture_queue_init(VideoPictureQueue *queue)
{
    memset(queue, 0, sizeof(VideoPictureQueue));
    queue->mutex = SDL_CreateMutex();
    queue->cond = SDL_CreateCond();
}

/* decode a frame from audio packet queue */
int audio_decode_frame(VideoState *video_state)
{
    AudioPacketDecodeCtx *audio_decode_ctx = &(video_state->audio_decode_ctx);
    AVFrame *frame = &(audio_decode_ctx->audio_frame);
    AVPacket *packet = &(audio_decode_ctx->audio_pkt);
    AVCodecContext *codec_ctx = video_state->audio_st->codec;
    int got_frame = 0;
    int data_size = 0;
    int len = 0;

    for (;;) {
        /* if packet still have un-decoded data */
        while (packet->size > 0){
            /* send packet to decoder */
            len = avcodec_decode_audio4(video_state->audio_st->codec, 
                                        frame, 
                                        &got_frame, 
                                        packet);
            if (len < 0) {
                /* error happen */
                av_log(NULL, AV_LOG_ERROR, "Failed to decode audio packet\n");
                break;
            }
            /* adjust packet data pointer */
            packet->size -= len;
            packet->data += len;

            /* copy data to buffer if got a audio frame */
            if (got_frame) {
                data_size = av_samples_get_buffer_size(NULL,
                                                 codec_ctx->channels,
                                                 frame->nb_samples,
                                                 codec_ctx->sample_fmt,
                                                 1);
                memcpy(audio_decode_ctx->audio_buf, frame->data[0], data_size);
                //av_log(NULL, AV_LOG_DEBUG, "audio frame have data_size %d\n", data_size);
            }
            if (data_size <= 0)
                continue;

            return data_size;
        }

        /* all data in packet have been send to decoder, free this useless packet */
        if (packet->data)
            av_free_packet(packet);   

        /* check quit */
        if (video_state->quit)
            return -1;

        /* get audio packet from queue */
        if (avpacket_queue_get(&(video_state->audio_queue), packet, 1) < 0)
            return -1;
    }
    return -1;
}

/* player callback requests for some decoded audio data */
void audio_callback(void *userdata, uint8_t * stream, int len)
{
    VideoState *video_state = (VideoState *)userdata;
    AudioPacketDecodeCtx *audio_decode_ctx = &(video_state->audio_decode_ctx);
    int decoded_auido_buf_size = 0;
    int copyed_len = 0;

    //av_log(NULL, AV_LOG_DEBUG, "audio_callback with len %d\n", len);

    while(len > 0)
    {
        /* audio buffer have data */
        if(audio_decode_ctx->audio_buf_index >= audio_decode_ctx->audio_buf_size)
        {
            /* audio buffer have no data */
            decoded_auido_buf_size = audio_decode_frame(video_state);
            if(decoded_auido_buf_size < 0)
            {
                /* If error, output silence */
                decoded_auido_buf_size = 1024;
                memset(audio_decode_ctx->audio_buf, 0x0, decoded_auido_buf_size);
            }
            else
            {
                audio_decode_ctx->audio_buf_size = decoded_auido_buf_size;
            }
            audio_decode_ctx->audio_buf_index = 0;
        }

        /* audio buffer now have data, copy to stream(player) */
        copyed_len = audio_decode_ctx->audio_buf_size - audio_decode_ctx->audio_buf_index;
        if(copyed_len > len)
            copyed_len = len;
        memcpy(stream, (uint8_t *)audio_decode_ctx->audio_buf + audio_decode_ctx->audio_buf_index, copyed_len);
        len -= copyed_len;
        stream += copyed_len;
        audio_decode_ctx->audio_buf_index += copyed_len;
    }
}


int video_queue_picture(VideoState *video_state, AVFrame *frame)
{
    static AVFrame *trans_frame = NULL;
    VideoPictureQueue *queue = &(video_state->video_picture_queue);
    
    SDL_LockMutex(queue->mutex);
    while (queue->nb_pictures >= VIDEO_PICTURE_QUEUE_SIZE) {
        SDL_CondWait(queue->cond, queue->mutex);
    }
    SDL_UnlockMutex(queue->mutex);

    if (video_state->quit)
        return -1;
        
    static uint8_t *buffer = NULL;
    if (trans_frame == NULL) {
        trans_frame = avcodec_alloc_frame();
        if (!trans_frame) {
            av_log(NULL, AV_LOG_ERROR, "failed to avcodec_alloc_frame\n");
        }
        int num_bytes = avpicture_get_size(PIX_FMT_RGB32, 360, 288);
    	buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
    	if (!buffer) {
    		av_log(NULL, AV_LOG_ERROR, "failed to alloc RGB video buffer\n");
    		return -1;
    	} 
    	if (avpicture_fill((AVPicture *)trans_frame, buffer, PIX_FMT_RGB32, 
    				360, 288)) {
		    av_log(NULL, AV_LOG_ERROR, "failed to avpicture_fill\n");
		}
        
    }

    if (video_state->sws_ctx) {
    if (sws_scale(video_state->sws_ctx,
              (const uint8_t * const*)frame->data, frame->linesize, 0, video_state->video_st->codec->height,
              trans_frame->data, trans_frame->linesize)) {
         av_log(NULL, AV_LOG_ERROR, "sws_scale failed\n");
    }
    }

    av_log(NULL, AV_LOG_DEBUG, "render texture to index %d\n", queue->windex);

    if (SDL_UpdateTexture(queue->picture[queue->windex], NULL, buffer, trans_frame->linesize[0]) < 0) {
         av_log(NULL, AV_LOG_ERROR, "render texture to index %d failed\n", queue->windex);
    }
    if (++queue->windex == VIDEO_PICTURE_QUEUE_SIZE)
        queue->windex = 0;

    SDL_LockMutex(queue->mutex);
    queue->nb_pictures++;
    SDL_UnlockMutex(queue->mutex);
    
    return 0;
}

int video_thread(void *args)
{
    VideoState *video_state = (VideoState *)args;
    AVCodecContext *video_codec_ctx = video_state->video_st->codec;
    
    AVPacket packet;
    int frame_finished = 0;
    AVFrame *decoded_frame = NULL;
                             
    /* create a frame to store decoded picture */
    decoded_frame = avcodec_alloc_frame();
    if (!decoded_frame) {
        av_log(NULL, AV_LOG_ERROR, "Failed to alloc a decoded frame in video thread\n");
        goto exit;
    }
    
    video_state->sws_ctx = sws_getContext(video_codec_ctx->width,
                                          video_codec_ctx->height,
                                          video_codec_ctx->pix_fmt,
                                          video_codec_ctx->width,
                                          video_codec_ctx->height, 
                                          PIX_FMT_YUV420P, 
                                          SWS_BILINEAR,
                                          NULL, NULL, NULL);
    if (!video_state->sws_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Failed to create sws context\n");
        goto exit;
    }

    // TODO: begin
    SDLContext *sdlctx = g_sdlctx;
    /* create main window and set background to black */
    sdlctx->window = SDL_CreateWindow(
                        video_state->filename,
                        SDL_WINDOWPOS_CENTERED, 
                        SDL_WINDOWPOS_CENTERED,
                        video_codec_ctx->width,
                        video_codec_ctx->height,
                        0);           
	if (!sdlctx->window) {
	    av_log(NULL, AV_LOG_ERROR, "Failed to create SDL Window\n");
	    exit(-1);
	}

	sdlctx->renderer = SDL_CreateRenderer(sdlctx->window, -1, 0);
	if (!sdlctx->renderer) {
	    av_log(NULL, AV_LOG_ERROR, "Failed to create video renderer\n");
	    exit(-1);
	}
	SDL_SetRenderDrawColor(sdlctx->renderer, 0, 0, 0, 255);
	SDL_RenderClear(sdlctx->renderer);
	SDL_RenderPresent(sdlctx->renderer);

    sdlctx->texture = SDL_CreateTexture(sdlctx->renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, video_codec_ctx->width, video_codec_ctx->height);
	if (!sdlctx->texture) {
	    av_log(NULL, AV_LOG_ERROR, "failed to create texture\n");
	    exit(1);
	}
    AVFrame *trans_frame = NULL;
    trans_frame = avcodec_alloc_frame();
    if (!trans_frame) {
        av_log(NULL, AV_LOG_ERROR, "failed to avcodec_alloc_frame\n");
        goto exit;
    }
    int num_bytes = avpicture_get_size(PIX_FMT_YUV420P, video_codec_ctx->width, video_codec_ctx->height);
	uint8_t *buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
	if (!buffer) {
		av_log(NULL, AV_LOG_ERROR, "failed to alloc RGB video buffer\n");
		goto exit;
	} 
	avpicture_fill((AVPicture *)trans_frame, buffer, PIX_FMT_YUV420P, 
				video_codec_ctx->width, video_codec_ctx->height);
	
    // TODO: end
    		
    for (;;) {
        if (avpacket_queue_get(&video_state->video_queue, &packet, 1) < 0) {
            break;
        }

        avcodec_decode_video2(
            video_state->video_st->codec, 
            decoded_frame, 
            &frame_finished,
            &packet);

        if (decoded_frame) {
            DEBUG();
            sws_scale(video_state->sws_ctx,
              (const uint8_t * const*)decoded_frame->data, decoded_frame->linesize, 0, video_codec_ctx->height,
              trans_frame->data, trans_frame->linesize);
            DEBUG();
            SDL_UpdateTexture(g_sdlctx->texture, NULL, buffer, trans_frame->linesize[0]);
            DEBUG();
            SDL_RenderClear(g_sdlctx->renderer);
			SDL_RenderCopy(g_sdlctx->renderer, 
			               g_sdlctx->texture, 
			               NULL, NULL);
			DEBUG();
			SDL_RenderPresent(g_sdlctx->renderer);
        }
        av_free_packet(&packet);
    }
    
exit:
    if (decoded_frame)
        av_free(decoded_frame);
    if (video_state->sws_ctx)
        sws_freeContext(video_state->sws_ctx);
        
    
    return 0;
}

int decode_thread(void *args)
{
    VideoState *video_state = (VideoState *)args;
    int i = 0, ret = 0;
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *video_codec_ctx = NULL;
    AVCodecContext *audio_codec_ctx = NULL;
    AVCodec *video_codec = NULL;
    AVCodec *audio_codec = NULL;
    AVPacket packet;

    /* open input */
    ret = avformat_open_input(
            &video_state->fmt_ctx,
            video_state->filename,
            NULL,
            NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open input %s\n", video_state->filename);
        goto exit;
    }

    /* find stream info */
    if (avformat_find_stream_info(video_state->fmt_ctx, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to find stream in file %s", video_state->filename);
        goto exit;
    }
    fmt_ctx = video_state->fmt_ctx;

    /* dump format info */
    av_dump_format(fmt_ctx, 0, video_state->filename, 0);

    /* open codec context */
    video_state->video_stream = -1;
    video_state->audio_stream = -1;
    for (i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && 
            video_state->video_stream < 0) {
            video_state->video_stream = i;
        }
        if (fmt_ctx->streams[i]->codec->codec_type== AVMEDIA_TYPE_AUDIO && 
            video_state->audio_stream < 0) {
            video_state->audio_stream = i;
        }
    }

    /* find and open video decoder */
    if (video_state->video_stream >= 0) {
        video_codec_ctx = fmt_ctx->streams[video_state->video_stream]->codec;
        video_codec = avcodec_find_decoder(video_codec_ctx->codec_id);
        if (video_codec == NULL) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find video decoder\n");
            goto exit;
        }
        if (avcodec_open2(video_codec_ctx, video_codec, NULL) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to open video decoder\n");
            goto exit;
        }
    } 

    /* find and open audio decoder */
    if (video_state->audio_stream >= 0) {
        audio_codec_ctx = fmt_ctx->streams[video_state->audio_stream]->codec;
        audio_codec = avcodec_find_decoder(audio_codec_ctx->codec_id);
        if (audio_codec == NULL) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find audio decoder\n");
            goto exit;
        }
        if (avcodec_open2(audio_codec_ctx, audio_codec, NULL) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to open audio decoder\n");
            goto exit;
        }
    }

    /* init video thread and queue */
    if (video_codec_ctx) {
        /* save stream pointer */
        video_state->video_st = fmt_ctx->streams[video_state->video_stream];
        
        /* init queue before video thread start */
        avpacket_queue_init(&video_state->video_queue);
        
        /* video thread */
        video_state->video_tid = SDL_CreateThread(video_thread, "videothread", video_state);
        if (!video_state->video_tid) {
            av_log(NULL, AV_LOG_ERROR, "Failed to create video thread\n");
            goto exit;
        }
    }

    /* init audio callbacks and queue */
    if (audio_codec_ctx) {
        SDL_AudioSpec wanted_spec, spec;
        wanted_spec.callback = audio_callback;
        wanted_spec.userdata = video_state;
        wanted_spec.format = AUDIO_S16SYS;
        wanted_spec.channels = audio_codec_ctx->channels;
        wanted_spec.silence = 0;
        wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;

        if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to open SDL audio\n");
            goto exit;
        }

        video_state->audio_st = fmt_ctx->streams[video_state->audio_stream];
        memset(&video_state->audio_decode_ctx, 0, sizeof(AudioPacketDecodeCtx));
        avpacket_queue_init(&video_state->audio_queue);
        SDL_PauseAudio(0);
    }

    /* read packet from format context and push into queue */
    for (;;) {
        if (video_state->quit) {
            break;
        }
        if (packet.data) {
            if (video_state->video_queue.size > MAX_VIDEOQ_SIZE) {
                SDL_Delay(10);
                continue;
            }
            if (video_state->audio_queue.size > MAX_AUDIOQ_SIZE) {
                SDL_Delay(10);
                continue;
            }
            if (packet.stream_index == video_state->video_stream) {
                avpacket_queue_put(&video_state->video_queue, &packet);
            } else if (packet.stream_index == video_state->audio_stream) {
                avpacket_queue_put(&video_state->audio_queue, &packet);
            } else {
                av_free_packet(&packet);
            }
        }

        if (av_read_frame(fmt_ctx, &packet) < 0) {
            if (fmt_ctx->pb->error == 0) {
                SDL_Delay(100);
                continue;
            } else {
                break;
            }
        }
    }

    /* decode thread done, all packet put into queue */
    while (!video_state->quit) {
        SDL_Delay(100);
    }
exit:
    if (video_codec_ctx)
        avcodec_close(video_codec_ctx);
    if (audio_codec_ctx)
        avcodec_close(audio_codec_ctx);
    if (video_state->fmt_ctx)
        avformat_close_input(&video_state->fmt_ctx);

    return 0;
}

unsigned int sdl_refresh_timer_cb(unsigned int interval, void *args)
{
    VideoState *video_state = (VideoState *)args;
    
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    event.user.data1 = args;
    SDL_PushEvent(&event);

    /* stop timer */
    return 0;
}

void schedule_refresh(VideoState *video_state, int delay)
{
    SDL_AddTimer(delay, sdl_refresh_timer_cb, video_state);
}



int video_refresh_timer(void *args)
{
    VideoState *video_state = (VideoState *)args;
    VideoPictureQueue *queue = &(video_state->video_picture_queue);
    
    if (video_state->video_st) {
        if (queue->nb_pictures == 0) {
            schedule_refresh(video_state, 1);
        } else {
            av_log(NULL, AV_LOG_DEBUG, "render texture from index %d\n", queue->rindex);
            SDL_Texture *texture = queue->picture[queue->rindex];
            schedule_refresh(video_state, 80);

            if (SDL_RenderClear(g_sdlctx->renderer) < 0) {
                av_log(NULL, AV_LOG_ERROR, "RenderClear failed\n");
            }
			if (SDL_RenderCopy(g_sdlctx->renderer, 
			               texture, 
			               NULL, NULL) < 0) {
			    av_log(NULL, AV_LOG_ERROR, "SDL_RenderCopy failed\n");
            }
			SDL_RenderPresent(g_sdlctx->renderer);

            /* update queue for next picture! */
            if(++(queue->rindex) == VIDEO_PICTURE_QUEUE_SIZE) {
                queue->rindex = 0;
            }
            SDL_LockMutex(queue->mutex);
            queue->nb_pictures--;
            SDL_CondSignal(queue->cond);
            SDL_UnlockMutex(queue->mutex);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int i   = 0;
	
	VideoState *video_state = NULL;
	SDLContext *sdlctx      = NULL;
	SDL_Event  event;

    /* argument check */
    if (argc != 2) {
		printf("%s [video_name]\n", argv[0]);
		exit(1);
	}
	
	/* setup debug level */
    av_log_set_level(AV_LOG_DEBUG);

    /* Init video_state */
	video_state = av_mallocz(sizeof(VideoState));
	if (!video_state) {
	    av_log(NULL, AV_LOG_ERROR, "Failed to create VideoState\n");
	    exit(1);
	}
    av_strlcpy(video_state->filename, argv[1], 1024);
	g_video_state = video_state;

    /* init sdlctx */
	sdlctx = av_mallocz(sizeof(SDLContext));
	if (!sdlctx) {
	    av_log(NULL, AV_LOG_ERROR, "Failed to create SDLContext\n");
	    exit(1);
	}
	g_sdlctx = sdlctx;

    /* init ffmpeg and SDL lib */
	avcodec_register_all();
	av_register_all();
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
	    av_log(NULL, AV_LOG_ERROR, "Failed to init SDL2\n");
	    exit(1);
	}



	/* create decode thread */
    video_state->decode_tid = SDL_CreateThread(decode_thread, "decode", video_state);
    if (!video_state->decode_tid) {
        av_log(NULL, AV_LOG_ERROR, "Failed to create decode thread\n");
        exit(1);
    }

    //schedule_refresh(video_state, 40);

    for (;;) {
        /* poll for event */
        SDL_WaitEvent(&event);
        switch(event.type) {
        case FF_QUIT_EVENT:
        case SDL_QUIT:
            video_state->quit = 1;
            goto quit;
        case FF_REFRESH_EVENT:
            video_refresh_timer(event.user.data1);
            break;
        default:
            break;
        }
    }

quit:
    SDL_WaitThread(video_state->decode_tid, NULL);

	if (sdlctx->window)
		SDL_DestroyWindow(sdlctx->window);
	if (sdlctx)
		av_free(sdlctx);

    if (video_state)
        av_free(video_state);

	return 0;
}
