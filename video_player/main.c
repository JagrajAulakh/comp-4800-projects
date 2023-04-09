#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
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
} VideoInfo;

AVFrame *abuf[CACHE_SIZE] = {NULL};
// Read and write pointers
unsigned int aw = 0, ar = 0;
// Initialize mutex lock for thread synchronization
pthread_mutex_t alock = PTHREAD_MUTEX_INITIALIZER;
// Initialize pthread_cond_t. Acts like a channel for thread sleeping/waking
pthread_cond_t acond = PTHREAD_COND_INITIALIZER;
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

	avcodec_parameters_to_context(
	    vi->video_codec_context,
	    vi->fmt_ctx->streams[vi->video_stream_index]->codecpar);
	avcodec_open2(vi->video_codec_context, vi->video_codec, NULL);

	avcodec_parameters_to_context(
	    vi->audio_codec_context,
	    vi->fmt_ctx->streams[vi->audio_stream_index]->codecpar);
	avcodec_open2(vi->audio_codec_context, vi->audio_codec, NULL);
	return vi;
}

// User will call this function to free a VideoInfo struct
void freeVideoInfo(VideoInfo *vi) {
	avformat_close_input(&vi->fmt_ctx);
	avcodec_free_context(&vi->video_codec_context);
	avcodec_free_context(&vi->audio_codec_context);
	free(vi);
}

// Producer thread that will push frames into the cache
void *decode_frames(void *arg) {
	const char *videoFilename = (char *)arg;

	// Get video information
	VideoInfo *vi = getVideoInfo(videoFilename);
	printf(
	    "Found video stream at index %d, audio stream at index %d, video "
	    "codec is %s, audio codec is %s, sample rate is %d\n",
	    vi->video_stream_index, vi->audio_stream_index,
	    vi->video_codec->name, vi->audio_codec->name,
	    vi->audio_codec_context->sample_rate);

	AVPacket *packet = av_packet_alloc();

	int frame_number = 0;
	// Read packets from the video file
	while (av_read_frame(vi->fmt_ctx, packet) >= 0) {
		// If a packet is apart of the video stream index we are
		// interested in:
		if (packet->stream_index == vi->audio_stream_index) {
			// Send the packet to the decoder
			int result = avcodec_send_packet(
			    vi->audio_codec_context, packet);
			// Error in decoding:
			if (result < 0) {
				fprintf(stderr, "ERROR DECODING PACKET\n");
				return 0;
			}

			while (result >= 0) {
				// Allocate AVFrame to store decoded frame
				AVFrame *frame = av_frame_alloc();
				result = avcodec_receive_frame(
				    vi->audio_codec_context, frame);
				if (result == AVERROR(EAGAIN)) {
					break;
				} else if (result == AVERROR_EOF) {
					printf("%d ERROR EOF\n", frame_number);
					break;
				} else if (result < 0) {
					fprintf(stderr,
					        "Failed to receive frame from "
					        "decoder: %s\n",
					        av_err2str(result));
					return 0;
				}

				frame_number++;

				// The is the sensitive section

				// Lock this section. Other threads CANNOT
				// interfere during this section
				pthread_mutex_lock(&alock);

				// Wait until buffer is not full
				while (aw == ar + CACHE_SIZE) {
					pthread_cond_wait(&acond, &alock);
				}

				// Push a frame into the buffer
				abuf[(aw++) % CACHE_SIZE] = frame;

				// Signal consumer that it's safe to read
				pthread_cond_signal(&acond);
				// Let go of the lock. Critical section is done
				pthread_mutex_unlock(&alock);
			}
		}
	}

	// Free resources related to video
	freeVideoInfo(vi);
	av_packet_free(&packet);

	// Signal the consumer to keep reading
	puts("Killing decode thread");
	pthread_cond_signal(&acond);
	// Set flag to indicate that video has ended
	producer_done = 1;
	return 0;
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

int main(int argc, char *argv[]) {
	if (argc < 2) {
		puts("Please give an video file");
		printf("Example: %s file.mp4\n", argv[0]);
		return 1;
	}

	makeDecodeThread(argv[1]);

	if (setupPulse() == -1) {
		pa_mainloop_free(m);
		pa_context_unref(pc);
		puts("Error setting up pulse connection");
		return 1;
	}

	if (pa_mainloop_run(m, NULL)) {
		fprintf(stderr, "pa_mainloop has quit.\n");
	}
}
