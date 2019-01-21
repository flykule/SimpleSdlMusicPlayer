#ifdef __cplusplus
#include <iostream>
using namespace std;
extern "C" {
#endif
#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#ifdef __cplusplus
}
#endif

//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  Uint8*  audio_chunk;
static  Uint32  audio_len;
static  Uint8*  audio_pos;
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

/* Audio Callback
 * The audio function callback takes the following parameters:
 * stream: A pointer to the audio buffer to be filled
 * len: The length (in bytes) of the audio buffer
 *
*/
void  fill_audio(void* udata, Uint8* stream, int len)
{
    //SDL 2.0
    SDL_memset(stream, 0, len);

    if (audio_len == 0)     /*  Only  play  if  we  have  data  left  */
    {
        return;
    }

    len = (len > audio_len ? audio_len : len); /*  Mix  as  much  data  as  possible  */

    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
}

int main(int argc, char* argv[])
{
    SDL_Window* screen;
    int got_picture;

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        fprintf(stderr, "Could not init sdl correctly: %s\n", SDL_GetError());
        return 1;
    }

    AVFormatContext* pFormatCtx = NULL;
    av_register_all();

    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
    {
        printf("Open file failed!\n");
        return 1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("Read stream info failed!\n");
        return -1;
    }

    av_dump_format(pFormatCtx, 0, argv[1], 0);
    AVCodec* codec = NULL;
    int audioStream = -1;
    audioStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

    if (!audioStream || codec == NULL)
    {
        printf("Get audio stream failed!\n");
        return -1;
    }

    printf("Audio stream index: %d\n", audioStream);

    AVCodecContext* avctx;
    avctx = pFormatCtx->streams[audioStream]->codec;

    if (avctx == NULL)
    {
        printf("Allocate av context failed!\n");
        return -1;
    }

    if (avcodec_open2(avctx, codec, NULL) < 0)
    {
        printf("Could not open codec!\n");
        return -1;
    }

    AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);

    //Out Audio Param
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    //nb_samples: AAC-1024 MP3-1152
    int out_nb_samples = avctx->frame_size;
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_sample_rate = 44100;
    int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
    //Out Buffer Size
    int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

    uint8_t* out_buffer = (uint8_t*)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
    AVFrame* pFrame = av_frame_alloc();

    //SDL_AudioSpec
    SDL_AudioSpec wanted_spec;
    SDL_AudioSpec spec;
    wanted_spec.freq = 44100;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = 2;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;
    wanted_spec.callback = fill_audio;
    wanted_spec.userdata = avctx;
    int64_t in_channel_layout;
    struct SwrContext* au_convert_ctx;

    printf("wanted spec: freq %d, channels %d\n", avctx->sample_rate, avctx->channels);

    if (SDL_OpenAudio(&wanted_spec, &spec) < 0)
    {
        fprintf(stderr, "SDL OpenAudio: %s\n!", SDL_GetError());
        return -1; //Could not open codec
    }

    //FIX:Some Codec's Context Information is missing
    in_channel_layout = av_get_default_channel_layout(avctx->channels);
    //Swr

    au_convert_ctx = swr_alloc();
    au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, (int64_t)out_channel_layout, out_sample_fmt, out_sample_rate,
                                        in_channel_layout, avctx->sample_fmt, avctx->sample_rate, 0, NULL);
    swr_init(au_convert_ctx);

    //Play
    SDL_PauseAudio(0);
    int index = 0;

    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
        if (packet->stream_index == audioStream)
        {
            int ret = avcodec_decode_audio4(avctx, pFrame, &got_picture, packet);

            if (ret < 0)
            {
                printf("Error in decoding audio frame.\n");
                return -1;
            }

            if (got_picture > 0)
            {
                swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t**)pFrame->data, pFrame->nb_samples);
                printf("index:%5d\t pts:%lld\t packet size:%d\n", index, (long long)packet->pts, packet->size);

                index++;
            }

            while (audio_len > 0) //Wait until finish
            {
                SDL_Delay(1);
            }

            //Set audio buffer (PCM data)
            audio_chunk = (Uint8*) out_buffer;
            //Audio buffer length
            audio_len = out_buffer_size;
            audio_pos = audio_chunk;

        }

        av_free_packet(packet);
    }

    swr_free(&au_convert_ctx);

    SDL_CloseAudio();//Close SDL
    SDL_Quit();

    av_free(out_buffer);
    avcodec_close(avctx);
    avformat_close_input(&pFormatCtx);

    return 0;

}
