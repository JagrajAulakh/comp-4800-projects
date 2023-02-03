#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#define FRAME_MAX 1

unsigned char rgbToGray1(unsigned char r, unsigned char g, unsigned char b) {
	// Using a standard formula
	double result = 0.299 * r + 0.587 * g + 0.114 * b;
	return (unsigned char)result;
}

unsigned char rgbToGray2(unsigned char r, unsigned char g, unsigned char b) {
	// Calculating average of RGB components
	double result = (r+g+b)/3.0;
	return (unsigned char)result;
}

unsigned char rgbToGray3(unsigned char r, unsigned char g, unsigned char b) {
	// Take average between max(r,g,b) and min(r,g,b)
	double max = ((r>g)?r:g)>b?g:b;
	double min = ((r<g)?r:g)<b?g:b;
	double result = (max+min)/2;
	return (unsigned char)result;
}

static void yuvFrameToRgbFrame(AVFrame *inputFrame, AVFrame *outputFrame) {
	// Input frame is in YUV format, convert to RGB24
	struct SwsContext *sws_ctx = sws_getContext(
	    inputFrame->width, inputFrame->height, inputFrame->format,
	    inputFrame->width, inputFrame->height, AV_PIX_FMT_RGBA, SWS_BICUBIC,
	    NULL, NULL, NULL);
	outputFrame->format = AV_PIX_FMT_RGB32;
	outputFrame->width = inputFrame->width;
	outputFrame->height = inputFrame->height;
	// Allocates memory for outputFrame->data, second argument is padding in
	// bytes
	av_frame_get_buffer(outputFrame, 32);

	sws_scale(sws_ctx, (const uint8_t *const *)inputFrame->data,
	          inputFrame->linesize, 0, inputFrame->height,
	          outputFrame->data, outputFrame->linesize);
}

static void pgm_save(AVFrame *inputFrame, const char *directory,
                     int frame_number) {
	FILE *f1, *f2, *f3;

	char filename1[200], filename2[200], filename3[200];
	sprintf(filename1, "%s/frame_%04d_01.pgm", directory, frame_number);
	sprintf(filename2, "%s/frame_%04d_02.pgm", directory, frame_number);
	sprintf(filename3, "%s/frame_%04d_03.pgm", directory, frame_number);

	f1 = fopen(filename1, "wb");
	f2 = fopen(filename2, "wb");
	f3 = fopen(filename3, "wb");

	fprintf(f1, "P5\n%d %d\n%d\n", inputFrame->width, inputFrame->height,
	        255);
	fprintf(f2, "P5\n%d %d\n%d\n", inputFrame->width, inputFrame->height,
	        255);
	fprintf(f3, "P5\n%d %d\n%d\n", inputFrame->width, inputFrame->height,
	        255);

	AVFrame *outputFrame = av_frame_alloc();
	yuvFrameToRgbFrame(inputFrame, outputFrame);

	const unsigned char *buf = outputFrame->data[0];

	int wrap = outputFrame->linesize[0];
	// printf("input wrap: %d, output wrap: %d\n", inputFrame->linesize[0],
	//        outputFrame->linesize[0]);
	printf("Writing frame %d\n", frame_number);
	for (int y = 0; y < inputFrame->height; y++) {
		for (int x = 0; x < inputFrame->width; x++) {
			const unsigned char r = *(buf + y * wrap + x * 4);
			const unsigned char g = *(buf + y * wrap + x * 4 + 1);
			const unsigned char b = *(buf + y * wrap + x * 4 + 2);

			unsigned char gray1, gray2, gray3;
			gray1 = rgbToGray1(r, g, b);
			gray2 = rgbToGray2(r, g, b);
			gray3 = rgbToGray3(r, g, b);
			fwrite(&gray1, sizeof(gray1), 1, f1);
			fwrite(&gray2, sizeof(gray2), 1, f2);
			fwrite(&gray3, sizeof(gray3), 1, f3);
		}
	}
	fclose(f1);
	fclose(f2);
	fclose(f3);
	av_frame_free(&outputFrame);
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

				pgm_save(frame, outdir, frame_number);
				frame_number++;
				if (FRAME_MAX != -1 &&
				    frame_number >= FRAME_MAX)
					goto done;
			}
		}
	}

done:

	printf("done\n");

	avformat_close_input(&fmt_ctx);
	avcodec_free_context(&codec_ctx);

	return 0;
}
