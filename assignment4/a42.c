#include <dirent.h>
#include <gtk/gtk.h>
#include <inttypes.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// decode packets into frames
GtkWidget *area;
cairo_surface_t *canvas;
int framenumber_requested;
void activate(GtkApplication *app, gpointer user_data);
AVFrame* yuv_to_rgbframe(int frame_number, AVFrame* yuv_frame);
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext,AVFrame *pFrame);
static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);
AVFrame *frame_buffer[150];

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("You need to specify a media file and a frame number:.\n");
        return -1;
    }
    framenumber_requested = atoi(argv[2]);
    char *argv2[] = {argv[0]};
    int argc2 = 1;
    AVFormatContext *pFormatContext = avformat_alloc_context();
    if (avformat_open_input(&pFormatContext, argv[1], NULL, NULL) != 0) {
        return -1;
    }
    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        return -1;
    }
    const AVCodec *pCodec = NULL;
    AVCodecParameters *pCodecParameters = NULL;
    int video_stream_index = -1;
    for (int i = 0; i < pFormatContext->nb_streams; i++) {
        AVCodecParameters *pLocalCodecParameters = NULL;
        pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
        const AVCodec *pLocalCodec = NULL;
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
        if (pLocalCodec == NULL) {
            continue;
        }
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (video_stream_index == -1) {
                video_stream_index = i;
                pCodec = pLocalCodec;
                pCodecParameters = pLocalCodecParameters;
            }
        }
    }
    if (video_stream_index == -1) {
        return -1;
    }
    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        return -1;
    }
    if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0) {
        return -1;
    }
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
        return -1;
    }
    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame) {
        return -1;
    }
    AVPacket *pPacket = av_packet_alloc();
    if (!pPacket) {
        return -1;
    }
    int response = 0;
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        if (pPacket->stream_index == video_stream_index) {
            response = decode_packet(pPacket, pCodecContext, pFrame);
            if (response < 0) break;
        }
        av_packet_unref(pPacket);
    }
    avformat_close_input(&pFormatContext);
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecContext);
    GtkApplication *app;
    int status;
    // making gtk window and app
    app = gtk_application_new("painter.org", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc2, argv2);
    g_object_unref(app);
    return status;
}


static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
  unsigned char *buf = frame_buffer[0]->data[0]; 
  int wrap = frame_buffer[0]->linesize[0];
  int xsize = frame_buffer[0]->width;
  int ysize = frame_buffer[0]->height;
  canvas = cairo_image_surface_create_for_data(buf, CAIRO_FORMAT_ARGB32, xsize, ysize, wrap);
  cairo_set_source_surface(cr, canvas, 0, 0);
  cairo_paint(cr);
}


AVFrame* yuv_to_rgbframe(int frame_number, AVFrame *yuv_frame){
    AVFrame *rgb_frame = NULL;
    struct SwsContext *sws_context = NULL;
    rgb_frame = av_frame_alloc();
    sws_context = sws_getContext(yuv_frame->width, yuv_frame->height, yuv_frame->format, yuv_frame->width, yuv_frame->height, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    rgb_frame->format = AV_PIX_FMT_RGB32;
    rgb_frame->width = yuv_frame->width;
    rgb_frame->height = yuv_frame->height;
    av_frame_get_buffer(rgb_frame, 32);
    sws_scale(sws_context, (const uint8_t * const *) yuv_frame->data, yuv_frame->linesize, 0, yuv_frame->height, rgb_frame->data, rgb_frame->linesize);
    return rgb_frame;

} 


// decode the packet
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext,AVFrame *pFrame) {
    // Supply raw packet data as input to a decoder
    int i = 0;
    int response = avcodec_send_packet(pCodecContext, pPacket);
    if (response < 0) {
        return response;
    }
    while (response >= 0) {
        // Return decoded output data (into a frame) from a decoder
        response = avcodec_receive_frame(pCodecContext, pFrame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            return response;
        }
        if (response >= 0) {
            // save a grayscale frame into a .pgm file
            if (pCodecContext->frame_number == framenumber_requested) {
                i++;
                frame_buffer[i] = yuv_to_rgbframe(pCodecContext->frame_number, pFrame);
                //gtk_widget_queue_draw(area);
                return -1;
            }
        }
    }
    return 0;
}
// saving frame as 5 different grayframes

void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window,*box;
    GtkWidget *image;
    window = gtk_application_window_new(app);
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_child(GTK_WINDOW(window), box);
    gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), image);
    gtk_window_set_title(GTK_WINDOW(window), "painter window");
    gtk_widget_show(window);
    area = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(area), 700);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), 700);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_function, NULL, NULL);
    gtk_widget_set_size_request(GTK_WIDGET(area), 300, 400);
    gtk_box_append(GTK_BOX(box), area);
}



