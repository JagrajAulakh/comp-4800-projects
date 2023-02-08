
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <inttypes.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <unistd.h>
#include <dirent.h>


// decode packets into frames
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);
// save a frame into a .pgm file
static void save_gray_frame(int frame_number, AVFrame *pFrame);

void activate(GtkApplication *app, gpointer user_data);

int framenumber_requested;

int main(int argc, const char *argv[])
{
  if (argc < 3) {
    printf("You need to specify a media file and a frame number:.\n");
    return -1;
  }
  framenumber_requested = atoi(argv[2]);
  char *argv2[] = {argv[0]};
  int argc2 = 1;


  

  AVFormatContext *pFormatContext = avformat_alloc_context();
  if (!pFormatContext) {
    return -1;
  }

  if (avformat_open_input(&pFormatContext, argv[1], NULL, NULL) != 0) {
    return -1;
  }


  if (avformat_find_stream_info(pFormatContext,  NULL) < 0) {
    return -1;
  }

  AVCodec *pCodec = NULL;
  AVCodecParameters *pCodecParameters =  NULL;
  int video_stream_index = -1;

  for (int i = 0; i < pFormatContext->nb_streams; i++)
  {
    AVCodecParameters *pLocalCodecParameters =  NULL;
    pLocalCodecParameters = pFormatContext->streams[i]->codecpar;

    AVCodec *pLocalCodec = NULL;


    pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

    if (pLocalCodec==NULL) {
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
  if (!pCodecContext)
  {
    return -1;
  }

  if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
  {
    return -1;
  }

  if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
  {
    return -1;
  }

  AVFrame *pFrame = av_frame_alloc();
  if (!pFrame)
  {
    return -1;
  }
  AVPacket *pPacket = av_packet_alloc();
  if (!pPacket)
  {
    return -1;
  }

  int response = 0;

  while (av_read_frame(pFormatContext, pPacket) >= 0)
  {
    if (pPacket->stream_index == video_stream_index) {

      response = decode_packet(pPacket, pCodecContext, pFrame);
      if (response < 0)
        break;
    }
    av_packet_unref(pPacket);
  }


  avformat_close_input(&pFormatContext);
  av_packet_free(&pPacket);
  av_frame_free(&pFrame);
  avcodec_free_context(&pCodecContext);
  GtkApplication *app;
  int status;
  //making gtk window and app
  app = gtk_application_new("painter.org",
                            G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc2, argv2);
  g_object_unref(app);
  return status;
}

//decode the packet 
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame)
{
  // Supply raw packet data as input to a decoder
  int response = avcodec_send_packet(pCodecContext, pPacket);

  if (response < 0) {
    return response;
  }

  while (response >= 0)
  {
    // Return decoded output data (into a frame) from a decoder
    response = avcodec_receive_frame(pCodecContext, pFrame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      return response;
    }

    if (response >= 0) {
      
      if (pFrame->format != AV_PIX_FMT_YUV420P)
      {
      }
      // save a grayscale frame into a .pgm file
      if(pCodecContext->frame_number == framenumber_requested){
        save_gray_frame(pCodecContext->frame_number, pFrame);
        return -1;
      }
    }
  }
  return 0;
}
//saving frame as 5 different grayframes
static void save_gray_frame(int frame_number, AVFrame *pFrame)
{
    FILE *fin;
    FILE *fin2;
    FILE *fin3;
    FILE *fin4;
    FILE *fin5;
    char frame_filename[1024];
    char frame_filename2[1024];
    char frame_filename3[1024];
    char frame_filename4[1024];
    char frame_filename5[1024];
    int i;
    unsigned char *buf;
    int wrap;
    int xsize;
    int ysize;
    int r;
    int g;
    int b;
    unsigned char grayscale_value;
    unsigned char grayscale_value2;
    unsigned char grayscale_value3;
    unsigned char grayscale_value4;
    unsigned char grayscale_value5;
    AVFrame *rgb_frame = NULL;
    struct SwsContext *sws_context = NULL;

    //make pgm names
    snprintf(frame_filename, sizeof(frame_filename), "%s-%d-1.pgm", "frame", frame_number);
    snprintf(frame_filename2, sizeof(frame_filename2), "%s-%d-2.pgm", "frame", frame_number);
    snprintf(frame_filename3, sizeof(frame_filename3), "%s-%d-3.pgm", "frame", frame_number);
    snprintf(frame_filename4, sizeof(frame_filename4), "%s-%d-4.pgm", "frame", frame_number);
    snprintf(frame_filename5, sizeof(frame_filename5), "%s-%d-5.pgm", "frame", frame_number);

    rgb_frame = av_frame_alloc();
    sws_context = sws_getContext(pFrame->width, pFrame->height, pFrame->format, pFrame->width, pFrame->height, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);
    rgb_frame->format = AV_PIX_FMT_RGBA;
    rgb_frame->width = pFrame->width;
    rgb_frame->height = pFrame->height;
    av_frame_get_buffer(rgb_frame, 32);
    sws_scale(sws_context, (const uint8_t * const *) pFrame->data, pFrame->linesize, 0, pFrame->height, rgb_frame->data, rgb_frame->linesize);
    buf = rgb_frame->data[0]; 
    wrap = rgb_frame->linesize[0];
    xsize = rgb_frame->width;
    ysize = rgb_frame->height;
    fin = fopen(frame_filename,"wb");
    fin2 = fopen(frame_filename2,"wb");
    fin3 = fopen(frame_filename3,"wb");
    fin4 = fopen(frame_filename4,"wb");
    fin5 = fopen(frame_filename5,"wb");

    //writing headers
    fprintf(fin, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    fprintf(fin2, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    fprintf(fin3, "P5\n%d %d\n%d\n", xsize, ysize, 255); 
    fprintf(fin4, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    fprintf(fin5, "P5\n%d %d\n%d\n", xsize, ysize, 255); 

    //calculating rgb values  and then calculating grayscale with 5 different formulas
    for(int j = 0; j<ysize; j++){
      for(i = 0; i<xsize; i++){               
          r = buf[(i*4) + (wrap*j)];
          g = buf[(i*4)+1 + (wrap*j)];                
          b = buf[(i*4)+2 + (wrap*j)];
          grayscale_value = (0.299 * r) + (0.587 * g) + (0.114*b);
          grayscale_value2 = (0.399 * r) + (0.487 * g) + (0.114*b);
          grayscale_value3 = (0.299 * r) + (0.637 * g) + (0.154*b);
          grayscale_value4 = (0.599 * r) + (0.387 * g) + (0.014*b);
          grayscale_value5 = (0.099 * r) + (0.687 * g) + (0.214*b);
          fwrite(&grayscale_value, sizeof(grayscale_value), 1, fin);
          fwrite(&grayscale_value2, sizeof(grayscale_value2), 1, fin2);
          fwrite(&grayscale_value3, sizeof(grayscale_value3), 1, fin3);
          fwrite(&grayscale_value4, sizeof(grayscale_value4), 1, fin4);
          fwrite(&grayscale_value5, sizeof(grayscale_value5), 1, fin5);

    }
  }
    fclose(fin);
    fclose(fin2);
    fclose(fin3);
    fclose(fin4);
    fclose(fin5);
}

void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window,*box;
  GtkWidget *image;
  GtkWidget *image2;
  GtkWidget *image3;
  GtkWidget *image4;
  GtkWidget *image5;
  char imagename[1024];
  char imagename2[1024];
  char imagename3[1024];
  char imagename4[1024];
  char imagename5[1024];
  snprintf(imagename, sizeof(imagename), "%s-%d-1.pgm", "frame", framenumber_requested);
  snprintf(imagename2, sizeof(imagename2), "%s-%d-2.pgm", "frame", framenumber_requested);
  snprintf(imagename3, sizeof(imagename3), "%s-%d-3.pgm", "frame", framenumber_requested);
  snprintf(imagename4, sizeof(imagename4), "%s-%d-4.pgm", "frame", framenumber_requested);
  snprintf(imagename5, sizeof(imagename5), "%s-%d-5.pgm", "frame", framenumber_requested);
  image = gtk_image_new_from_file(imagename);
  image2 = gtk_image_new_from_file(imagename2);
  image3 = gtk_image_new_from_file(imagename3);
  image4 = gtk_image_new_from_file(imagename4);
  image5 = gtk_image_new_from_file(imagename5);
  gtk_image_set_pixel_size(GTK_IMAGE(image), 400);
  gtk_image_set_pixel_size(GTK_IMAGE(image2), 400);
  gtk_image_set_pixel_size(GTK_IMAGE(image3), 400);
  gtk_image_set_pixel_size(GTK_IMAGE(image4), 400);
  gtk_image_set_pixel_size(GTK_IMAGE(image5), 400);
  window = gtk_application_window_new(app);
  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
  gtk_window_set_child(GTK_WINDOW(window), box);
  gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(image2, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(image2, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(image3, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(image3, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(image4, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(image4, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(image5, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(image5, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(box), image);
  gtk_box_append(GTK_BOX(box), image2);
  gtk_box_append(GTK_BOX(box), image3);
  gtk_box_append(GTK_BOX(box), image4);
  gtk_box_append(GTK_BOX(box), image5);
  gtk_window_set_title(GTK_WINDOW(window), "painter window");
  gtk_widget_show(window);
}