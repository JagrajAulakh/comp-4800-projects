#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     const char *directory, int frame_number) {
	FILE *f;
	int i;

	char filename[200];
	sprintf(filename, "%s/%04d.pgm", directory, frame_number);
	f = fopen(filename, "wb");
	fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
	for (i = 0; i < ysize; i++) fwrite(buf + i * wrap, 1, xsize, f);
	fclose(f);
}

int main(int argc, char **argv) {
	if (argc <= 2) {
		fprintf(stderr,
		        "Usage: %s <input file> <output directory>\n"
		        "THIS WILL OUTPUT ALL FRAMES OF THE VIDEO: BEWARE!\n",
		        argv[0]);
		exit(0);
	}
	const char *filename = argv[1], *outdir = argv[2];

	if (mkdir(outdir, 0777) < 0 && errno != EEXIST) {
		printf("Error making outputdir, error=%d, EEXIST=%d\n", errno,
		       EEXIST);
		return EXIT_FAILURE;
	}
	AVFormatContext *fmt_ctx = NULL;
	// Open video file
	avformat_open_input(&fmt_ctx, filename, NULL, NULL);
	// Retrieve the stream info and write it fmt_ctx.
	avformat_find_stream_info(fmt_ctx, NULL);

	// Find the video stream index
	const AVCodec *codec = NULL;
	int video_stream_idx = -1;
	video_stream_idx =
	    av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
	printf("Found video stream at index %d, codec is %s\n",
	       video_stream_idx, codec->name);

	AVCodecContext *codec_ctx = NULL;
	codec_ctx = avcodec_alloc_context3(codec);

	avcodec_parameters_to_context(
	    codec_ctx, fmt_ctx->streams[video_stream_idx]->codecpar);
	avcodec_open2(codec_ctx, codec, NULL);

	AVFrame *frame = av_frame_alloc();
	AVPacket *packet = av_packet_alloc();

	int frame_number = 0;
	// Read packets from the video file
	while (av_read_frame(fmt_ctx, packet) >= 0) {
		// If a packet is apart of the video stream index we are
		// interested in:
		if (packet->stream_index == video_stream_idx) {
			// Send the packet to the decoder
			int result = avcodec_send_packet(codec_ctx, packet);
			// Error in decoding:
			if (result < 0) {
				return -1;
			}

			while (result >= 0) {
				result =
				    avcodec_receive_frame(codec_ctx, frame);
				if (result == AVERROR(EAGAIN)) {
					break;
				} else if (result == AVERROR_EOF) {
					printf("%d ERROR EOF\n", frame_number);
					break;
				} else if (result < 0) {
					return -1;
				}
				pgm_save(frame->data[0], frame->linesize[0],
				         frame->width, frame->height, outdir,
				         frame_number++);
				if (frame_number > 10) goto done;
			}
		}
	}

done:

	printf("joe\n");

	avformat_close_input(&fmt_ctx);
	avcodec_free_context(&codec_ctx);

	return 0;
}
