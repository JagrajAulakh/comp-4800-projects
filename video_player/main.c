#include <gtk/gtk.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

// How many frames to hold in cache. Make sure it more than 2
#define CACHE_SIZE 100

// Hold video header information
typedef struct {
	AVFormatContext *fmt_ctx;
	AVCodecContext *codec_context;
	const AVCodec *video_codec;
	int video_stream_index;
} VideoInfo;

AVFrame *circ_buf[CACHE_SIZE] = {NULL};
unsigned int w = 0, r = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_cond = PTHREAD_COND_INITIALIZER;
int producer_done = 0;

// Convert a frame in YUV format (or any format) to RGB32 format
static void yuvFrameToRgbFrame(AVFrame *inputFrame, AVFrame *outputFrame) {
	struct SwsContext *sws_ctx = sws_getContext(
	    inputFrame->width, inputFrame->height, inputFrame->format,
	    inputFrame->width, inputFrame->height, AV_PIX_FMT_RGB32,
	    SWS_BICUBIC, NULL, NULL, NULL);

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

// Get header information for a video file. It's up to the caller to free this
// struct
VideoInfo *getVideoInfo(const char *filename) {
	VideoInfo *vi = (VideoInfo *)malloc(sizeof(VideoInfo));
	vi->fmt_ctx = NULL;
	vi->codec_context = NULL;
	vi->video_stream_index = -1;
	vi->video_codec = NULL;

	avformat_open_input(&vi->fmt_ctx, filename, NULL, NULL);
	avformat_find_stream_info(vi->fmt_ctx, NULL);

	vi->video_stream_index = av_find_best_stream(
	    vi->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &vi->video_codec, 0);

	vi->codec_context = avcodec_alloc_context3(vi->video_codec);

	avcodec_parameters_to_context(
	    vi->codec_context,
	    vi->fmt_ctx->streams[vi->video_stream_index]->codecpar);
	avcodec_open2(vi->codec_context, vi->video_codec, NULL);
	return vi;
}

// User will call this function to free a VideoInfo struct
void freeVideoInfo(VideoInfo *vi) {
	avformat_close_input(&vi->fmt_ctx);
	avcodec_free_context(&vi->codec_context);
	free(vi);
}

// Producer thread that will push frames into the cache
void *decode_frames(void *arg) {
	const char *videoFilename = (char *)arg;
	VideoInfo *vi = getVideoInfo(videoFilename);
	printf("Found video stream at index %d, codec is %s\n",
	       vi->video_stream_index, vi->video_codec->name);

	AVPacket *packet = av_packet_alloc();

	int frame_number = 0;
	// Read packets from the video file
	while (av_read_frame(vi->fmt_ctx, packet) >= 0) {
		// If a packet is apart of the video stream index we are
		// interested in:
		if (packet->stream_index == vi->video_stream_index) {
			// Send the packet to the decoder
			int result =
			    avcodec_send_packet(vi->codec_context, packet);
			// Error in decoding:
			if (result < 0) {
				fprintf(stderr, "ERROR DECODING PACKET\n");
				return 0;
			}

			while (result >= 0) {
				AVFrame *frame = av_frame_alloc();
				result = avcodec_receive_frame(
				    vi->codec_context, frame);
				if (result == AVERROR(EAGAIN)) {
					break;
				} else if (result == AVERROR_EOF) {
					printf("%d ERROR EOF\n", frame_number);
					break;
				} else if (result < 0) {
					return 0;
				}

				// Count how many frames we've seen so far
				frame_number++;
				// printf(
				//     "time: %lu, frame_numer: %d, w=%d,
				//     r=%d\n", frame->best_effort_timestamp,
				//     frame_number, w, r);

				pthread_mutex_lock(&lock);
				while (w == r + CACHE_SIZE) {
					pthread_cond_wait(&full_cond, &lock);
				}
				circ_buf[(w++) % CACHE_SIZE] = frame;

				pthread_cond_signal(&full_cond);
				pthread_mutex_unlock(&lock);
			}
		}
	}

	freeVideoInfo(vi);
	av_packet_free(&packet);
	puts("Killing decode thread");
	pthread_cond_signal(&full_cond);
	producer_done = 1;
	return 0;
}

// Consumer thread that will pop and free frames from the cache
AVFrame *prevFrame = NULL, *prevRgbFrame = NULL;
void draw(GtkDrawingArea *drawingArea, cairo_t *cr, int width, int height,
          gpointer data) {
	AVFrame *frame = NULL;

	pthread_mutex_lock(&lock);

	while (r == w &&
	       !producer_done) {  // Buffer is empty and producer hasn't stopped
		pthread_cond_wait(&full_cond, &lock);
	}

	frame = r == w && producer_done ? NULL : circ_buf[(r++) % CACHE_SIZE];

	pthread_cond_signal(&full_cond);
	pthread_mutex_unlock(&lock);

	// If there is indeed a frame in the queue:
	if (frame != NULL) {
		// Convert YUV frame data to RGB so cairo can understand
		AVFrame *rgbFrame = av_frame_alloc();
		yuvFrameToRgbFrame(frame, rgbFrame);
		cairo_surface_t *frameSurface =
		    cairo_image_surface_create_for_data(
		        rgbFrame->data[0], CAIRO_FORMAT_ARGB32, rgbFrame->width,
		        rgbFrame->height, rgbFrame->linesize[0]);

		// Draw frame
		cairo_set_source_surface(cr, frameSurface, 0, 0);
		cairo_paint(cr);

		// Free previous frame (cairo_paint still needs current frame
		// data allocated, so we free the memory for the previously
		// drawn frame)
		if (prevFrame != NULL) {
			av_frame_free(&prevFrame);
		}
		if (prevRgbFrame != NULL) {
			av_frame_free(&prevRgbFrame);
		}
		prevFrame = frame;
		prevRgbFrame = rgbFrame;
	} else {  // If there was no frame in queue, draw black screen
		cairo_rectangle(cr, 0, 0, width, height);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_fill(cr);
	}
}

void makeDecodeThread(const char *filename) {
	pthread_t tid;
	pthread_create(&tid, NULL, decode_frames, (void *)filename);
}

void activate(GtkApplication *app, gpointer data) {
	char *filename = (char *)data;

	// Get video information
	VideoInfo *vi = getVideoInfo(filename);
	int framerate =
	    vi->fmt_ctx->streams[vi->video_stream_index]->r_frame_rate.num /
	    vi->fmt_ctx->streams[vi->video_stream_index]->r_frame_rate.den;
	int videoWidth = vi->codec_context->width,
	    videoHeight = vi->codec_context->height;

	printf("Dimensions are %dx%d, framerate is %d\n", videoWidth,
	       videoHeight, framerate);

	GtkWidget *window, *box, *drawingArea;

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Video player");
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	drawingArea = gtk_drawing_area_new();
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawingArea), draw,
	                               NULL, NULL);
	gtk_widget_set_size_request(drawingArea, videoWidth, videoHeight);

	// Setup drawing thread (consumer)
	g_timeout_add(1000 / framerate, (GSourceFunc)gtk_widget_queue_draw,
	              drawingArea);
	// Setup decoding thread (producer)
	makeDecodeThread(filename);

	gtk_box_append(GTK_BOX(box), drawingArea);
	gtk_window_set_child(GTK_WINDOW(window), box);
	gtk_widget_show(window);

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
	g_signal_connect(app, "activate", G_CALLBACK(activate),
	                 (void *)filename);
	status = g_application_run(G_APPLICATION(app), 1, newArgv);

	return 0;
}
