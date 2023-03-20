#include <stdio.h>
#include <pulse/volume.h>
#include <pulse/context.h>
#include <pulse/mainloop.h>

static int i = 0;

void callback(pa_context *c, void *userdata) {
	printf("calledback %d\n", ++i);
}

int main(int argc, char *argv[]) {
	pa_mainloop *m = pa_mainloop_new();
	pa_mainloop_api *mainloop_api = pa_mainloop_get_api(m);

	pa_context* pc = pa_context_new(mainloop_api, argv[0]);

	if (pa_context_connect(pc, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
		fprintf(stderr, "Pulse audio context failed to connect\n");
		goto quit;
	}
	puts("Connected to pulseaudio context successfully");


	pa_context_set_state_callback(pc, callback, NULL);
	
	int ret;
	if (pa_mainloop_run(m, &ret)) {
        fprintf(stderr, "pa_mainloop_run() failed.\n");
	}


quit:
	pa_mainloop_free(m);
	pa_context_unref(pc);
}
