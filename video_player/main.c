#include <gtk/gtk.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
	AVFormatContext *fmt_ctx;
	AVCodecContext *codec_context;
	const AVCodec *video_codec;
	int video_stream_index;
} VideoInfo;

void setWidgetCss(GtkWidget *widget, const char *css) {
	GtkCssProvider *provider = gtk_css_provider_new();
	const char *data = css;
	gtk_css_provider_load_from_data(provider, data, -1);
	GdkDisplay *displej = gtk_widget_get_display(GTK_WIDGET(widget));
	gtk_style_context_add_provider_for_display(
	    displej, GTK_STYLE_PROVIDER(provider),
	    GTK_STYLE_PROVIDER_PRIORITY_USER);
}

// Convert a frame in YUV format (or any format) to RGB32 format
static void yuvFrameToRgbFrame(AVFrame *inputFrame, AVFrame *outputFrame) {
	struct SwsContext *sws_ctx = sws_getContext(
	    inputFrame->width, inputFrame->height, inputFrame->format,
	    inputFrame->width, inputFrame->height, AV_PIX_FMT_RGBA, SWS_BICUBIC,
	    NULL, NULL, NULL);

	// Set the outputFrame's configuration options
	outputFrame->format = AV_PIX_FMT_RGB32;
	outputFrame->width = inputFrame->width;
	outputFrame->height = inputFrame->height;
	// Allocates memory for outputFrame->data, second argument is padding in
	// bytes
	av_frame_get_buffer(outputFrame, 32);

	// sws_scale will actually do the conversion
	sws_scale(sws_ctx, (const uint8_t *const *)inputFrame->data,
	          inputFrame->linesize, 0, inputFrame->height,
	          outputFrame->data, outputFrame->linesize);
}

VideoInfo *getVideoInfo(const char *filename) {
	VideoInfo *vi = (VideoInfo *)malloc(sizeof(VideoInfo));
	vi->fmt_ctx = NULL;
	vi->codec_context = NULL;
	vi->video_stream_index = -1;
	vi->video_codec = NULL;

	avformat_open_input(&vi->fmt_ctx, filename, NULL, NULL);
	avformat_find_stream_info(vi->fmt_ctx, NULL);

	vi->video_stream_index= av_find_best_stream(vi->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &vi->video_codec, 0);

	vi->codec_context = avcodec_alloc_context3(vi->video_codec);

	avcodec_parameters_to_context(
	    vi->codec_context, vi->fmt_ctx->streams[vi->video_stream_index]->codecpar);
	avcodec_open2(vi->codec_context, vi->video_codec, NULL);
	return vi;
}

void freeVideoInfo(VideoInfo *vi) {
	avformat_close_input(&vi->fmt_ctx);
	avcodec_free_context(&vi->codec_context);
	free(vi);
}

int outputFrames(const char *videoFilename, const char *outdir, int target_frame) {
	VideoInfo *videoInfo = getVideoInfo(videoFilename);
	// Find the video stream index

	printf("Found video stream at index %d, codec is %s\n", videoInfo->video_stream_index, videoInfo->video_codec->name);

	AVFrame *frame = av_frame_alloc();
	AVPacket *packet = av_packet_alloc();

	int frame_number = 0;
	// Read packets from the video file
	// while (av_read_frame(fmt_ctx, packet) >= 0) {
	// 	// If a packet is apart of the video stream index we are
	// 	// interested in:
	// 	if (packet->stream_index == video_stream_idx) {
	// 		// Send the packet to the decoder
	// 		int result = avcodec_send_packet(codec_ctx, packet);
	// 		// Error in decoding:
	// 		if (result < 0) {
	// 			return -1;
	// 		}
	//
	// 		while (result >= 0) {
	// 			result =
	// 			    avcodec_receive_frame(codec_ctx, frame);
	// 			if (result == AVERROR(EAGAIN)) {
	// 				break;
	// 			} else if (result == AVERROR_EOF) {
	// 				printf("%d ERROR EOF\n", frame_number);
	// 				break;
	// 			} else if (result < 0) {
	// 				return 0;
	// 			}
	//
	// 			// Count how many frames we've seen so far
	// 			frame_number++;
	// 			// If we reach target frame, save it and exit
	// 			if (frame_number == target_frame) {
	// 				avformat_close_input(&fmt_ctx);
	// 				avcodec_free_context(&codec_ctx);
	// 				return 1;
	// 			}
	// 		}
	// 	}
	// }

	freeVideoInfo(videoInfo);
	return 0;
}

void draw(GtkDrawingArea *drawingArea, cairo_t *cr, int width, int height, gpointer data) {
	cairo_set_source_rgb(cr, 1, 0, 0);
	cairo_fill(cr);
}

void activate(GtkApplication *app, gpointer data) {
	char *filename = (char *)data;
	GtkWidget *window, *box, *drawingArea;

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Video player");

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	drawingArea = gtk_drawing_area_new();
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawingArea), draw, NULL, NULL);

	VideoInfo *vi = getVideoInfo(filename);
	printf("Found video stream at index %d, codec is %s\n", vi->video_stream_index, vi->video_codec->name);

	int framerate = vi->fmt_ctx->streams[vi->video_stream_index]->r_frame_rate.num;
	int videoWidth = vi->codec_context->width, videoHeight = vi->codec_context->height;
	printf("Dimensions are %dx%d, framerate is %d\n", videoWidth, videoHeight, framerate);

	g_timeout_add(1000/framerate, (GSourceFunc)gtk_widget_queue_draw, drawingArea);

	gtk_box_append(GTK_BOX(box), drawingArea);
	gtk_window_set_child(GTK_WINDOW(window), box);
	gtk_widget_show(window);

	puts("Freeing video info");
	freeVideoInfo(vi);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "\n\nUsage: %s <input file>\n", argv[0]);
		exit(0);
	}
	const char *filename = argv[1];

	GtkApplication *app;
	int status;
	app = gtk_application_new("com.jagrajaulakh.frames",
	                          G_APPLICATION_DEFAULT_FLAGS);

	char *newArgv[] = {argv[0]};
	g_signal_connect(app, "activate", G_CALLBACK(activate), (void *)filename);
	status = g_application_run(G_APPLICATION(app), 1, newArgv);

	return 0;
}
