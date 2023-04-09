#include <gtk/gtk.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <math.h>
#include <pthread.h>
#include <pulse/pulseaudio.h>
#include <stdio.h>

#define CACHE_SIZE 100

// Hold video header information
typedef struct {
	AVFormatContext *fmt_ctx;
	AVCodecContext *video_codec_context;
	AVCodecContext *audio_codec_context;
	const AVCodec *video_codec;
	const AVCodec *audio_codec;
	int video_stream_index;
	int audio_stream_index;
	int framerate, width, height;
} VideoInfo;

AVFrame *abuf[CACHE_SIZE] = {NULL}, *vbuf[CACHE_SIZE] = {NULL};
// Read and write pointers
unsigned int aw = 0, ar = 0, vw = 0, vr = 0;
// Initialize mutex lock for thread synchronization
pthread_mutex_t alock = PTHREAD_MUTEX_INITIALIZER,
                vlock = PTHREAD_MUTEX_INITIALIZER;
// Initialize pthread_cond_t. Acts like a channel for thread sleeping/waking
pthread_cond_t acond = PTHREAD_COND_INITIALIZER,
               vcond = PTHREAD_COND_INITIALIZER;
// Flag that determines when the video has reached the end
int producer_done = 0;

pa_stream *stream;
pa_context *pc;
pa_sample_spec spec;
pa_mainloop *m;

VideoInfo *getVideoInfo(const char *filename) {
	VideoInfo *vi = (VideoInfo *)malloc(sizeof(VideoInfo));
	vi->fmt_ctx = NULL;
	vi->video_codec_context = NULL;
	vi->audio_codec_context = NULL;
	vi->video_stream_index = -1;
	vi->audio_stream_index = -1;
	vi->video_codec = NULL;
	vi->audio_codec = NULL;

	avformat_open_input(&vi->fmt_ctx, filename, NULL, NULL);
	avformat_find_stream_info(vi->fmt_ctx, NULL);

	vi->video_stream_index = av_find_best_stream(
	    vi->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &vi->video_codec, 0);
	vi->audio_stream_index = av_find_best_stream(
	    vi->fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &vi->audio_codec, 0);

	vi->video_codec_context = avcodec_alloc_context3(vi->video_codec);
	vi->audio_codec_context = avcodec_alloc_context3(vi->audio_codec);


	vi->framerate =
	    vi->fmt_ctx->streams[vi->video_stream_index]->r_frame_rate.num /
	    vi->fmt_ctx->streams[vi->video_stream_index]->r_frame_rate.den;

	avcodec_parameters_to_context(
	    vi->video_codec_context,
	    vi->fmt_ctx->streams[vi->video_stream_index]->codecpar);
	avcodec_open2(vi->video_codec_context, vi->video_codec, NULL);

	avcodec_parameters_to_context(
	    vi->audio_codec_context,
	    vi->fmt_ctx->streams[vi->audio_stream_index]->codecpar);
	avcodec_open2(vi->audio_codec_context, vi->audio_codec, NULL);

	vi->width = vi->video_codec_context->width;
	vi->height = vi->video_codec_context->height;
	return vi;
}

// User will call this function to free a VideoInfo struct
void freeVideoInfo(VideoInfo *vi) {
	avformat_close_input(&vi->fmt_ctx);
	avcodec_free_context(&vi->video_codec_context);
	avcodec_free_context(&vi->audio_codec_context);
	free(vi);
}

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

// Producer thread that will push frames into the cache
void *decode_frames(void *arg) {
	const char *videoFilename = (char *)arg;

	// Get video information
	VideoInfo *vi = getVideoInfo(videoFilename);
	printf(
	    "Found video stream at index %d, audio stream at index %d, video "
	    "codec is %s, audio codec is %s, sample rate is %d, framerate is "
	    "%d\n",
	    vi->video_stream_index, vi->audio_stream_index,
	    vi->video_codec->name, vi->audio_codec->name,
	    vi->audio_codec_context->sample_rate, vi->framerate);

	AVPacket *packet = av_packet_alloc();

	int vcount = 0, acount = 0;
	while (av_read_frame(vi->fmt_ctx, packet) >= 0) {
		int result;
		int is_audio;
		if (packet->stream_index == vi->audio_stream_index) {
			result = avcodec_send_packet(vi->audio_codec_context,
			                             packet);
			is_audio = 1;
		} else if (packet->stream_index == vi->video_stream_index) {
			result = avcodec_send_packet(vi->video_codec_context,
			                             packet);
			is_audio = 0;
		} else {
			continue;
		}

		if (result < 0) {
			fprintf(stderr, "ERROR DECODING PACKET\n");
			return 0;
		}

		while (result >= 0) {
			// Allocate AVFrame to store decoded frame
			AVFrame *frame = av_frame_alloc();
			if (is_audio) {
				result = avcodec_receive_frame(
				    vi->audio_codec_context, frame);
			} else {
				result = avcodec_receive_frame(
				    vi->video_codec_context, frame);
			}
			if (result == AVERROR(EAGAIN)) {
				break;
			} else if (result == AVERROR_EOF) {
				break;
			} else if (result < 0) {
				fprintf(stderr,
				        "Failed to receive frame from "
				        "decoder: %s\n",
				        av_err2str(result));
				return 0;
			}

			if (is_audio) {
				pthread_mutex_lock(&alock);
				while (aw == ar + CACHE_SIZE) {
					// puts("abuf full, waiting");
					pthread_cond_wait(&acond, &alock);
				}
				abuf[(aw++) % CACHE_SIZE] = frame;
				// printf("Pushing audio frame %d\n", acount++);
				pthread_cond_signal(&acond);
				pthread_mutex_unlock(&alock);
			} else {
				pthread_mutex_lock(&vlock);
				while (vw == vr + CACHE_SIZE) {
					// puts("vbuf full, waiting");
					pthread_cond_wait(&vcond, &vlock);
				}
				vbuf[(vw++) % CACHE_SIZE] = frame;
				// printf("Pushing video frame %d\n", vcount++);
				pthread_cond_signal(&vcond);
				pthread_mutex_unlock(&vlock);
			}
		}
	}

	// Free resources related to video
	freeVideoInfo(vi);
	av_packet_free(&packet);

	// Signal consumers to keep reading
	puts("Killing decode thread");
	pthread_cond_signal(&acond);
	pthread_cond_signal(&vcond);
	// Set flag to indicate that video has ended
	producer_done = 1;
	return 0;
}

AVFrame *prevFrame = NULL, *prevRgbFrame = NULL;
void draw(GtkDrawingArea *drawingArea, cairo_t *cr, int width, int height,
          gpointer data) {
	AVFrame *frame = NULL;

	// Lock this section. Other threads CANNOT interfere during this section
	pthread_mutex_lock(&vlock);
	// Wait until buffer is not empty (there's actually data to read)
	while (vr == vw &&
	       !producer_done) {  // Buffer is empty and producer hasn't stopped
		puts("[Consumer] Buffer is empty! Waiting...");
		pthread_cond_wait(&vcond, &vlock);
	}

	// Retrieve a frame from the buffer, or NULL if video is done and buffer
	// is empty
	frame = vr == vw && producer_done ? NULL : vbuf[(vr++) % CACHE_SIZE];

	// Signal producer that it's safe to write to the buffer
	pthread_cond_signal(&vcond);
	// Let go of the lock. Critical section is done
	pthread_mutex_unlock(&vlock);

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

		// Set the current frame that was drawn as the previous frame
		// for the next draw iteration
		prevFrame = frame;
		prevRgbFrame = rgbFrame;
	} else {  // If there was no frame in buffer, draw black screen
		cairo_rectangle(cr, 0, 0, width, height);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_fill(cr);
	}
}

void state_callback(pa_context *c, void *userdata) {
	int *pa_ready = userdata;
	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_READY:
			*pa_ready = 1;
			break;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			pa_mainloop_free(m);
			break;
		default:
			break;
	}
}

void write_callback(pa_stream *s, size_t length, void *userdata) {
	AVFrame *frame = NULL;

	pthread_mutex_lock(&alock);
	while (ar == aw &&
	       !producer_done) {  // Buffer is empty and producer hasn't stopped
		pthread_cond_wait(&acond, &alock);
	}

	frame = ar == aw && producer_done ? NULL : abuf[(ar++) % CACHE_SIZE];

	pthread_cond_signal(&acond);
	pthread_mutex_unlock(&alock);

	if (frame) {
		pa_stream_write(s, frame->extended_data[0], frame->linesize[0],
		                NULL, 0, PA_SEEK_RELATIVE);
		av_frame_free(&frame);
	}

	if (ar == aw && producer_done) {
		pa_mainloop_quit(m, 0);
	}
}

void sink_info_callback(pa_context *pc, pa_sink_info *info, int eol,
                        void *userdata) {
	if (info) {
		pa_cvolume *v = &info->volume;
		pa_volume_t volume = pa_cvolume_avg(v);
		double percentage =
		    (100 * (double)volume / (double)PA_VOLUME_NORM);
		printf("Device name: %s\n", info->name);
		printf("Volume: %.2f%%\n", percentage);
		printf("Sample rate: %dHz\n", info->sample_spec.rate);
	}

	if (eol) {
		return;
	}
}

int setupPulse() {
	m = pa_mainloop_new();
	pa_mainloop_api *mainloop_api = pa_mainloop_get_api(m);
	pc = pa_context_new(mainloop_api, "playback");

	if (pa_context_connect(pc, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
		fprintf(stderr, "Pulse audio context failed to connect\n");
		return -1;
	}
	puts("Connected to pulseaudio context successfully");

	// Wait for pulse to be ready
	int pa_ready = 0;
	pa_context_set_state_callback(pc, state_callback, &pa_ready);
	while (pa_ready == 0) {
		pa_mainloop_iterate(m, 1, NULL);
	}

	pa_context_get_sink_info_list(pc, (void *)sink_info_callback, NULL);

	spec.format = PA_SAMPLE_FLOAT32LE;
	spec.rate = 48000;
	spec.channels = 1;

	// Create a new playback stream
	if (!(stream = pa_stream_new(pc, "playback", &spec, NULL))) {
		puts("pa_stream_new failed");
		return -1;
	}

	pa_buffer_attr bufattr;
	bufattr.fragsize = (uint32_t)-1;
	bufattr.maxlength = 8192;
	bufattr.minreq = 0;
	bufattr.prebuf = 0;
	bufattr.tlength = 8192;

	pa_stream_connect_playback(stream, NULL, &bufattr,
	                           PA_STREAM_INTERPOLATE_TIMING |
	                               PA_STREAM_ADJUST_LATENCY |
	                               PA_STREAM_AUTO_TIMING_UPDATE,
	                           NULL, NULL);

	pa_stream_set_write_callback(stream, write_callback, NULL);
	return 0;
}

// Makes the producer thread
void makeDecodeThread(const char *filename) {
	pthread_t tid;
	pthread_create(&tid, NULL, decode_frames, (void *)filename);
}

void startPulseMainLoop() {
	if (setupPulse() == -1) {
		pa_mainloop_free(m);
		pa_context_unref(pc);
		puts("Error setting up pulse connection");
		return;
	}

	if (pa_mainloop_run(m, NULL)) {
		fprintf(stderr, "pa_mainloop has quit.\n");
	}
}

void makeAudioThread() {
	pthread_t tid;
	pthread_create(&tid, NULL, (void *)startPulseMainLoop, NULL);
}
void activate(GtkApplication *app, gpointer data) {
	char *filename = (char *)data;

	// Get video information
	VideoInfo *vi = getVideoInfo(filename);

	printf("Dimensions are %dx%d, framerate is %d\n", vi->width, vi->height,
	       vi->framerate);

	GtkWidget *window, *box, *drawingArea;

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Video player");
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	drawingArea = gtk_drawing_area_new();
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawingArea), draw,
	                               NULL, NULL);
	// Resize window to dimensions of the video
	gtk_widget_set_size_request(drawingArea, vi->width, vi->height);

	// Setup drawing thread (consumer)
	g_timeout_add(1000 / vi->framerate, (GSourceFunc)gtk_widget_queue_draw,
	              drawingArea);
	// Setup decoding thread (producer)
	makeDecodeThread(filename);
	makeAudioThread();

	gtk_box_append(GTK_BOX(box), drawingArea);
	gtk_window_set_child(GTK_WINDOW(window), box);
	gtk_widget_show(window);

	freeVideoInfo(vi);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		puts("Please give an video file");
		printf("Example: %s file.mp4\n", argv[0]);
		return 1;
	}
	const char *filename = argv[1];
	GtkApplication *app;
	int status;
	app = gtk_application_new("com.jagrajaulakh.final",
	                          G_APPLICATION_DEFAULT_FLAGS);

	char *newArgv[] = {argv[0]};
	g_signal_connect(app, "activate", G_CALLBACK(activate),
	                 (void *)filename);
	status = g_application_run(G_APPLICATION(app), 1, newArgv);

	return 0;
}
