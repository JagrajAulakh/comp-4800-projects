#include <gtk/gtk.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

struct Params {
	const char *videoFilename;
	const char *outputDir;
	int targetFrame;
};

void setWidgetCss(GtkWidget *widget, const char *css) {
	GtkCssProvider *provider = gtk_css_provider_new();
	const char *data = css;
	gtk_css_provider_load_from_data(provider, data, -1);
	GdkDisplay *displej = gtk_widget_get_display(GTK_WIDGET(widget));
	gtk_style_context_add_provider_for_display(
	    displej, GTK_STYLE_PROVIDER(provider),
	    GTK_STYLE_PROVIDER_PRIORITY_USER);
}

// --------------------------
// Converting RGB to grayscale values
// --------------------------

unsigned char rgbToGray1(unsigned char r, unsigned char g, unsigned char b) {
	// Using a standard formula
	double result = 0.299 * r + 0.587 * g + 0.114 * b;
	return (unsigned char)result;
}

unsigned char rgbToGray2(unsigned char r, unsigned char g, unsigned char b) {
	// Calculating average of RGB components
	double result = (r + g + b) / 3.0;
	return (unsigned char)result;
}

unsigned char rgbToGray3(unsigned char r, unsigned char g, unsigned char b) {
	// Take average between max(r,g,b) and min(r,g,b)
	double max = ((r > g) ? r : g) > b ? g : b;
	double min = ((r < g) ? r : g) < b ? g : b;
	double result = (max + min) / 2;
	return (unsigned char)result;
}

unsigned char rgbToGray4(unsigned char r, unsigned char g, unsigned char b) {
	// Using a standard formula with different coefficients
	double result = 0.35 * r + 0.60 * g + 0.10 * b;
	return (unsigned char)result;
}

unsigned char rgbToGray5(unsigned char r, unsigned char g, unsigned char b) {
	// Using a standard formula with different coefficients
	double result = 0.1 * r + 0.7 * g + 0.04 * b;
	return (unsigned char)result;
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

// Saves 5 pgm images given an single frame, output dir, and the frame number
static void pgm_save(AVFrame *inputFrame, const char *directory,
                     int frame_number) {
	FILE *f1, *f2, *f3, *f4, *f5;

	// Create the output filenames for the 5 images
	char filename1[200], filename2[200], filename3[200], filename4[20],
	    filename5[200];
	sprintf(filename1, "%s/frame_%04d_01.pgm", directory, frame_number);
	sprintf(filename2, "%s/frame_%04d_02.pgm", directory, frame_number);
	sprintf(filename3, "%s/frame_%04d_03.pgm", directory, frame_number);
	sprintf(filename4, "%s/frame_%04d_04.pgm", directory, frame_number);
	sprintf(filename5, "%s/frame_%04d_05.pgm", directory, frame_number);

	// Open 5 files for writing, binary (since we are using P5 header)
	f1 = fopen(filename1, "wb");
	f2 = fopen(filename2, "wb");
	f3 = fopen(filename3, "wb");
	f4 = fopen(filename4, "wb");
	f5 = fopen(filename5, "wb");

	// Write headers into 5 files
	fprintf(f1, "P5\n%d %d\n%d\n", inputFrame->width, inputFrame->height,
	        255);
	fprintf(f2, "P5\n%d %d\n%d\n", inputFrame->width, inputFrame->height,
	        255);
	fprintf(f3, "P5\n%d %d\n%d\n", inputFrame->width, inputFrame->height,
	        255);
	fprintf(f4, "P5\n%d %d\n%d\n", inputFrame->width, inputFrame->height,
	        255);
	fprintf(f5, "P5\n%d %d\n%d\n", inputFrame->width, inputFrame->height,
	        255);

	// Allocate output frame for the RGB conversion
	AVFrame *outputFrame = av_frame_alloc();
	// Convert input frame colors to RGB
	yuvFrameToRgbFrame(inputFrame, outputFrame);

	// Retrieve raw data from outputFrame, which hold the colors in RGB32
	// format
	const unsigned char *buf = outputFrame->data[0];

	int wrap = outputFrame->linesize[0];
	printf("Writing frame %d\n", frame_number);

	// Go through every pixel in the frame
	for (int y = 0; y < inputFrame->height; y++) {
		for (int x = 0; x < inputFrame->width; x++) {
			// Retieve rgb values
			const unsigned char r = *(buf + y * wrap + x * 4);
			const unsigned char g = *(buf + y * wrap + x * 4 + 1);
			const unsigned char b = *(buf + y * wrap + x * 4 + 2);

			// Convert rgb to grayscale using 5 different algorithms
			unsigned char gray1, gray2, gray3, gray4, gray5;
			gray1 = rgbToGray1(r, g, b);
			gray2 = rgbToGray2(r, g, b);
			gray3 = rgbToGray3(r, g, b);
			gray4 = rgbToGray4(r, g, b);
			gray5 = rgbToGray5(r, g, b);
			// Write 5 grayscale values to 5 output files
			fwrite(&gray1, sizeof(gray1), 1, f1);
			fwrite(&gray2, sizeof(gray2), 1, f2);
			fwrite(&gray3, sizeof(gray3), 1, f3);
			fwrite(&gray4, sizeof(gray4), 1, f4);
			fwrite(&gray5, sizeof(gray5), 1, f5);
		}
	}
	fclose(f1);
	fclose(f2);
	fclose(f3);
	fclose(f4);
	fclose(f5);
	// Dealocate the outupt frame that holds the converted RGB32 data
	av_frame_free(&outputFrame);
}

int makeOutputDir(const char *outdir) {
	if (mkdir(outdir, 0777) < 0 && errno != EEXIST) {
		fprintf(stderr, "Error making outputdir, error=%d, EEXIST=%d\n",
		        errno, EEXIST);
		return 0;
	}
	return 1;
}

int outputFrames(const char *videoFilename, const char *outdir,
                 int target_frame) {
	AVFormatContext *fmt_ctx = NULL;
	// Open video file
	avformat_open_input(&fmt_ctx, videoFilename, NULL, NULL);
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
					return 0;
				}

				// Count how many frames we've seen so far
				frame_number++;
				// If we reach target frame, save it and exit
				if (frame_number == target_frame) {
					pgm_save(frame, outdir, frame_number);
					avformat_close_input(&fmt_ctx);
					avcodec_free_context(&codec_ctx);
					return 1;
				}
			}
		}
	}

	avformat_close_input(&fmt_ctx);
	avcodec_free_context(&codec_ctx);
	return 0;
}

void activate(GtkApplication *app, struct Params *params) {
	GtkWidget *window, *box;
	GtkWidget *pictures[5];

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Grayscale frames");

	box = gtk_flow_box_new();

	for (int i = 0; i < 5; i++) {
		char filename[200], labelString[10];
		GtkWidget *picture, *label;
		sprintf(filename, "%s/frame_%04d_%02d.pgm", params->outputDir,
		        params->targetFrame, i + 1);
		sprintf(labelString, "%d", i + 1);
		pictures[i] = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
		picture = gtk_picture_new_for_filename(filename);
		label = gtk_label_new(labelString);
		gtk_widget_set_name(label, "label");
		gtk_picture_set_content_fit(GTK_PICTURE(picture), GTK_CONTENT_FIT_CONTAIN);
		gtk_box_append(GTK_BOX(pictures[i]), label);
		gtk_box_append(GTK_BOX(pictures[i]), picture);
	}

	for (int i = 0; i < 5; i++) {
		gtk_flow_box_append(GTK_FLOW_BOX(box), *(pictures + i));
	}

	gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(box), GTK_SELECTION_NONE);
	gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(box), 1);
	gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(box), 3);
	gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(box), 3);
	gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(box), 32);
	gtk_widget_set_valign(box, GTK_ALIGN_START);

	gtk_window_set_child(GTK_WINDOW(window), box);
	setWidgetCss(window, "#label { font-size: 32px; }");

	gtk_widget_show(window);
	free(params);
}

int main(int argc, char **argv) {
	if (argc <= 2) {
		fprintf(stderr,
		        "\n\nUsage: %s <input file> <output directory> [frame "
		        "number]\n"
		        "Output directory will be created if doesn't exist\n"
		        "Frame number is optional, default is first frame\n\n",
		        argv[0]);
		exit(0);
	}
	const char *filename = argv[1], *outdir = argv[2];
	int target_frame = (argc == 3 ? 1 : atoi(argv[3]));
	printf("argc=%d, target_frame=%d\n", argc, target_frame);

	if (!makeOutputDir(outdir)) {
		return EXIT_FAILURE;
	}

	if (!outputFrames(filename, outdir, target_frame)) {
		return EXIT_FAILURE;
	}

	GtkApplication *app;
	int status;
	app = gtk_application_new("com.jagrajaulakh.frames",
	                          G_APPLICATION_DEFAULT_FLAGS);

	struct Params *params = malloc(sizeof(struct Params *));
	params->outputDir = outdir;
	params->videoFilename = filename;
	params->targetFrame = target_frame;

	char *newArgv[] = {argv[1]};
	g_signal_connect(app, "activate", G_CALLBACK(activate), params);
	status = g_application_run(G_APPLICATION(app), 1, newArgv);

	return 0;
}
