#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define CHANNEL 3
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

// ascii density map or the concentration map
// we will see how luminous the pixel is and then 
// assign the character form this map
static const char *ascii_map = "@%#*+=-:. ";
int map_len = 10;

/// @brief the rendering mode for image and videoooo
typedef enum {mASCII,mUNICODE} renderMode;
typedef enum {cNONE,cRGB} colorMode;
typedef enum {mVideo,mImage} format;

typedef struct {
    const char *file;
    int out_w;
    int out_h;
    renderMode mode;
    colorMode color;
    format f;
    int fps;
} config;

// i will use this later but yea i didnnt yet use this
typedef struct{
    int* x_map;
    int* y_map;
    size_t buf_capacity;
    char* out_buf;
}precomp;

static void print_usage() {
    printf(
        "Example usage: mocha -i <image> [options]\n"
        "\nOptions:\n"
        "  -i <path>        input image\n"
        "  -v <path>        input video\n"
        "  -w <width>       output width (cols)\n"
        "  -h <height>      output height (rows)\n"
        "  --mode ascii     ASCII rendering\n"
        "  --mode unicode   unicode block rendering\n"
        "  --color none     black & white\n"
        "  --color rgb      truecolor output\n"
        "  --fps val        output fps\n"
        "  --help           show this message\n"
    );
}

// we wil need this to manage the fps so yea
static void sleep_ms(int ms){
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// maybe i should optimize this
static void clear_screen() {write(STDOUT_FILENO, "\x1b[H\x1b[2J", 7);}

void ascii_print(uint8_t *img, int w, int h, config* cfg) {
    const float aspect = 0.5f;
    if(cfg->out_h == 0) {
        cfg->out_h = (int)(h * cfg->out_w / (float)w * aspect);
    }

    if(cfg->color == cRGB){
        for(int i = 0; i < cfg->out_h; i++){
            for(int j = 0; j < cfg->out_w; j++){
                int idx_j = j * w / cfg->out_w;
                int idx_i = i * h / cfg->out_h;
                int idx = (idx_i * w + idx_j) * 3;

                unsigned char R = img[idx + 0];
                unsigned char G = img[idx + 1];
                unsigned char B = img[idx + 2];

                float L =
                    0.2126f*(R/255.0f) +
                    0.7152f*(G/255.0f) +
                    0.0722f*(B/255.0f);

                L = powf(L, 1.0f / 2.2f);
                int ci = (int)((1.0f - L) * map_len);

                printf("\x1b[38;2;%u;%u;%um%c", R, G, B, ascii_map[ci]);
            }
            printf("\x1b[0m\n");
        }
    } else {
        for(int i = 0; i < cfg->out_h; i++){
            for(int j = 0; j < cfg->out_w; j++){
                int idx_j = j * w / cfg->out_w;
                int idx_i = i * h / cfg->out_h;
                int idx = (idx_i * w + idx_j) * 3;

                unsigned char R = img[idx + 0];
                unsigned char G = img[idx + 1];
                unsigned char B = img[idx + 2];

                float L =
                    0.2126f*(R/255.0f) +
                    0.7152f*(G/255.0f) +
                    0.0722f*(B/255.0f);

                L = powf(L, 1.0f / 2.2f);
                int ci = (int)((1.0f - L) * map_len);

                printf("%c", ascii_map[ci]);
            }
            printf("\n");
        }
    }
}

void unicode_print_rgb(uint8_t *img, int w, int h, config* cfg, precomp* ops) {
    // printf("we entered the upr function\n");
    char* p = ops->out_buf;
    p += sprintf(p, "\x1b[H");
    // we are using this aspect to make the video fit the terminal and ascii height to width 
    // ps i find it perfect on my terminal tho i am using the half block
    const float aspect = 0.5f;
    if(cfg->out_h == 0){
        cfg->out_h = (int)(h* cfg->out_w / (float)w * aspect);
    }

    for(int i = 0; i < cfg->out_h; i++){
        for(int j = 0; j < cfg->out_w; j++){
            int out_j = j * w / cfg->out_w;

            int i0 = (2*i) * h / (cfg->out_h * 2);
            int i1 = (2*i + 1) * h / (cfg->out_h * 2);

            /*
                _______________
                | i0  | i0 + 1 |
                _______________
                | i1  | i1 + 1 |
                _______________

            */
            i0 = (i0*w + out_j) * CHANNEL;
            i1 = (i1*w + out_j) * CHANNEL;

            unsigned char r0 = img[i0 + 0];
            unsigned char g0 = img[i0 + 1];
            unsigned char b0 = img[i0 + 2];

            unsigned char r1 = img[i1 + 0];
            unsigned char g1 = img[i1 + 1];
            unsigned char b1 = img[i1 + 2];

            p += sprintf(p,"\x1b[38;2;%u;%u;%um\x1b[48;2;%u;%u;%um▀", r0, g0, b0, r1, g1, b1);
            // printf("\x1b[48;2;%u;%u;%um", r1, g1, b1);
            // printf("▀");
        }
        p += sprintf(p,"\x1b[0m\n");
    }
    fflush(stdout);

    write(STDOUT_FILENO, ops->out_buf, p - ops->out_buf);
}

void print_image(config* cfg){
    int h, w, c;
    unsigned char* img = stbi_load(cfg->file, &w, &h, &c, CHANNEL);
    
    if(!img){
        printf("sorry there was a issue with loading image\n");
        return;
    }

    // 1. MUST calculate height before allocating ops maps
    if(cfg->out_h == 0) {
        const float aspect = 0.5f;
        cfg->out_h = (int)(h * cfg->out_w / (float)w * aspect);
    }

    precomp ops_struct; 
    precomp* ops = &ops_struct;

    // 2. Allocate and fill maps
    ops->x_map = malloc(cfg->out_w * sizeof(int));
    ops->y_map = malloc(cfg->out_h * 2 * sizeof(int));
    ops->out_buf = malloc(cfg->out_h * cfg->out_w * 128 + 2048); // Increased buffer size for safety

    for(int j = 0; j < cfg->out_w; j++){
        ops->x_map[j] = (j * w / cfg->out_w) * CHANNEL;
    }
    for(int i = 0; i < cfg->out_h * 2; i++){
        ops->y_map[i] = (i * h / (cfg->out_h * 2)) * w * CHANNEL;
    }

    clear_screen();
    if(cfg->mode == mASCII) {
        ascii_print(img, w, h, cfg);
    } else {
        // 3. PASS 'ops' INSTEAD OF '0'
        unicode_print_rgb(img, w, h, cfg, ops);
    }

    // Clean up
    free(ops->x_map);
    free(ops->y_map);
    free(ops->out_buf);
    stbi_image_free(img);
}

// currently there is a buffering issue so i have to think how we improve the write speed to be better 
void print_video(config* cfg) {
    avformat_network_init();

    AVFormatContext *fmt_ctx = NULL;
    if (avformat_open_input(&fmt_ctx, cfg->file, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open video file\n");
        return;
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream info\n");
        return;
    }

    int video_stream_index = -1;
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        fprintf(stderr, "No video stream found\n");
        return;
    }

    AVCodecParameters *codecpar = fmt_ctx->streams[video_stream_index]->codecpar;
    AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codecpar);
    avcodec_open2(codec_ctx, codec, NULL);

    AVFrame *frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();

    int out_w = cfg->out_w;
    int out_h = cfg->out_h;

    

    if (out_h == 0) {
        out_h = (int)(codec_ctx->height * out_w / (float)codec_ctx->width * 0.5f);
        cfg->out_h = out_h;
    }

    struct SwsContext *sws_ctx = sws_getContext(
        codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
        out_w, out_h, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL
    );

    int rgb_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, out_w, out_h, 1);
    uint8_t *rgb_buffer = (uint8_t*)av_malloc(rgb_size);
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, rgb_buffer,
                         AV_PIX_FMT_RGB24, out_w, out_h, 1);

    AVPacket packet;
    av_init_packet(&packet);

    precomp ops_struct; // Allocate on stack
    precomp* ops = &ops_struct; // Pointer to that stack memory

    ops->x_map = malloc(cfg->out_w * sizeof(int));
    ops->y_map = malloc(cfg->out_h * 2 * sizeof(int));
    ops->out_buf = malloc(cfg->out_h * cfg->out_w * 64 + 1024);
        for(int j = 0; j < cfg->out_w; j++){
            ops->x_map[j] = (j* out_w / cfg->out_w) * CHANNEL;
        }
        for(int i = 0; i < cfg->out_h*2; i++){
            ops->y_map[i] = (i* out_h / (cfg->out_h *2)) * out_w * CHANNEL;
        }
        ops->buf_capacity = cfg->out_h * cfg->out_w * 64 + 1204;
        // ops->out_buf = malloc(ops->buf_capacity);
    while (av_read_frame(fmt_ctx, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            avcodec_send_packet(codec_ctx, &packet);

            while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                sws_scale(sws_ctx, (const uint8_t * const*)frame->data,
                          frame->linesize, 0, codec_ctx->height,
                          rgb_frame->data, rgb_frame->linesize);

                clear_screen();
                if(cfg->mode == mASCII) {
                    ascii_print(rgb_frame->data[0], out_w, out_h, cfg);
                } else {
                    unicode_print_rgb(rgb_frame->data[0], out_w, out_h, cfg, ops);
                }

                sleep_ms(1000 / cfg->fps);
            }
        }
        av_packet_unref(&packet);
    }

    av_free(rgb_buffer);
    av_frame_free(&rgb_frame);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    free(ops->x_map);
    free(ops->y_map);
}

void print_image_or_video(config* cfg){
    if(cfg->f == mVideo){
        print_video(cfg);
    } else {
        print_image(cfg);
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        print_usage();
        return 0;
    }

    config cfg = {0};

    cfg.out_w = 50;
    cfg.out_h = 0;
    cfg.mode  = mUNICODE;
    cfg.color = cRGB;
    cfg.f     = mImage;
    cfg.fps   = 30;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            cfg.file = argv[++i];
            cfg.f = mImage;
        }
        else if (strcmp(argv[i], "-v") == 0 && i + 1 < argc) {
            cfg.file = argv[++i];
            cfg.f = mVideo;
        }
        else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            cfg.out_w = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            cfg.out_h = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
           if (strcmp(argv[i+1], "ascii") == 0) {
               cfg.mode = mASCII;
           } else if (strcmp(argv[i+1], "unicode") == 0) {
               cfg.mode = mUNICODE;
           } else {
               fprintf(stderr, "Unknown mode: %s\n", argv[i+1]);
               return 1;
           }
           i++;
        }
        else if (strcmp(argv[i], "--color") == 0 && i + 1 < argc) {
           if (strcmp(argv[i+1], "RGB") == 0) {
               cfg.color = cRGB;
           } else if (strcmp(argv[i+1], "none") == 0) {
               cfg.color = cNONE;
           } else {
               fprintf(stderr, "Unknown mode: %s\n", argv[i+1]);
               return 1;
           }
           i++;
        }
        else if (strcmp(argv[i], "--fps") == 0 && i + 1 < argc) {
            cfg.fps = atoi(argv[++i]);
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    if (!cfg.file) {
        fprintf(stderr, "No input file provided\n");
        return 1;
    }

    print_image_or_video(&cfg);
    return 0;
}
