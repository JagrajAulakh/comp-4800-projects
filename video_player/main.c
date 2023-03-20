#include <math.h>
#include <pulse/context.h>
#include <pulse/mainloop.h>
#include <pulse/stream.h>
#include <pulse/volume.h>
#include <stdio.h>

static int freq = 440;

pa_stream *stream;
pa_context *pc;
pa_sample_spec spec;
pa_mainloop *m;

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

void writeSoundData(int8_t *buf, size_t length, double freq) {
	for (int i = 0; i <= length / 2; i += 2) {
		double sample_d =
		    0.2 * SHRT_MAX * sin(2 * M_PI * freq * (double)i / 44100.0);
		int16_t sample = sample_d;
		buf[i] = sample & 0xff;
		buf[i + 1] = (sample >> 8) & 0xff;
	}
}

void write_callback(pa_stream *s, size_t length, void *userdata) {
	int8_t *buf = userdata;

	pa_stream_write(s, buf, length, NULL, 0, PA_SEEK_RELATIVE);
	// writeSoundData(buf, 4000, freq++);
}

int main(int argc, char *argv[]) {
	m = pa_mainloop_new();
	pa_mainloop_api *mainloop_api = pa_mainloop_get_api(m);
	pc = pa_context_new(mainloop_api, "playback");

	if (pa_context_connect(pc, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
		fprintf(stderr, "Pulse audio context failed to connect\n");
		goto quit;
	}
	puts("Connected to pulseaudio context successfully");

	// Wait for pulse to be ready
	int pa_ready = 0;
	pa_context_set_state_callback(pc, state_callback, &pa_ready);
	while (pa_ready == 0) {
		pa_mainloop_iterate(m, 1, NULL);
	}

	spec.format = PA_SAMPLE_S16LE;
	spec.rate = 44100;
	spec.channels = 1;

	int8_t buf[30000];
	writeSoundData(buf, 30000, freq);

	// Create a new playback stream
	if (!(stream = pa_stream_new(pc, "playback", &spec, NULL))) {
		puts("pa_stream_new failed");
		goto quit;
	}

	pa_buffer_attr bufattr;
	bufattr.fragsize = (uint32_t)-1;
	bufattr.maxlength = 30000;
	bufattr.minreq = 0;
	bufattr.prebuf = 100;
	bufattr.tlength = 30000;

	pa_stream_connect_playback(stream, NULL, &bufattr,
	                           PA_STREAM_INTERPOLATE_TIMING |
	                               PA_STREAM_ADJUST_LATENCY |
	                               PA_STREAM_AUTO_TIMING_UPDATE,
	                           NULL, NULL);

	pa_stream_set_write_callback(stream, write_callback, (void *)buf);

	if (pa_mainloop_run(m, NULL)) {
		fprintf(stderr, "pa_mainloop_run() failed.\n");
	}

quit:
	pa_mainloop_free(m);
	pa_context_unref(pc);
}
