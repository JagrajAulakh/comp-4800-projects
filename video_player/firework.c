#include "firework.h"

#include <gtk/gtk.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

#include "queue.h"

struct firework {
	GTimer *timers[20];
	int x, y, num_of_angles;
	double r, g, b, angles[20];
	int max_duration;
	Queue *q;
};

Firework *firework_new(Queue *q, int x, int y) {
	Firework *f = (Firework *)malloc(sizeof(Firework));

	f->x = x;
	f->y = y;

	int num_of_angles = 3 + rand() % 7;
	f->num_of_angles = num_of_angles;
	for (int i = 0; i < num_of_angles; i++) {
		f->timers[i] = g_timer_new();
		g_timer_start(f->timers[i]);

		double angle = (double)(rand() % 10000) / 10000.0 * 2 * M_PI;
		f->angles[i] = angle;
	}

	f->timers[num_of_angles] = NULL;
	f->angles[num_of_angles] = -1;
	f->max_duration = rand() % 800 + 800;
	f->q = q;

	firework_random_color(f);
	firework_make_thread(f);
	return f;
}

void firework_random_color(Firework *f) {
	f->r = (double)(rand() % 10000) / 10000;
	f->g = (double)(rand() % 10000) / 10000;
	f->b = (double)(rand() % 10000) / 10000;
}

int firework_get_x(Firework *f) { return f->x; }
int firework_get_y(Firework *f) { return f->y; }
int firework_get_r(Firework *f) { return f->r; }
int firework_get_g(Firework *f) { return f->g; }
int firework_get_b(Firework *f) { return f->b; }
int firework_get_num_of_angles(Firework *f) { return f->num_of_angles; }

void firework_print(Firework *f) {
	printf(
	    "{Firework: x=%d,y=%d, num_of_angles=%d, color=(%.2f,%.2f,%.2f)}",
	    f->x, f->y, f->num_of_angles, f->r, f->g, f->b);
}

double *firework_get_angles(Firework *f) { return f->angles; }

double firework_get_percentage(Firework *f, int i) {
	double elapsed = g_timer_elapsed(f->timers[i], NULL) * 1000;
	double p = elapsed / (double)f->max_duration;
	return p;
}

void firework_draw(cairo_t *cr, Firework *f) {
	cairo_set_source_rgb(cr, firework_get_r(f), firework_get_g(f),
	                     firework_get_b(f));
	for (int i = 0; i < f->num_of_angles; i++) {
		GTimer *t = f->timers[i];
		if (g_timer_is_active(t)) {
			double percentage1 = firework_get_percentage(f, i);
			double percentage2 =
			    percentage1 * percentage1 * percentage1;

			// printf("i=%d, percentage1=%.2f, percentage2=%.2f\n",
			// i,
			//        percentage1, percentage2);
			cairo_set_source_rgb(cr, f->r, f->g, f->b);
			cairo_move_to(
			    cr,
			    f->x + (f->max_duration / 50) * percentage2 *
			               cos(f->angles[i]),
			    f->y + (f->max_duration / 50) * percentage2 *
			               sin(f->angles[i]));
			cairo_line_to(
			    cr,
			    f->x + (f->max_duration / 50) * percentage1 *
			               cos(f->angles[i]),
			    f->y + (f->max_duration / 50) * percentage1 *
			               sin(f->angles[i]));
			cairo_stroke(cr);
		}
	}
}

struct monitorargs {
	Firework *f;
	int i;
};

void *monitor_timer(void *arg) {
	struct monitorargs *ma = (struct monitorargs *)arg;
	Firework *f = ma->f;
	int i = ma->i;

	if (f->timers[i] == NULL) {
		fprintf(stderr, "Timer is null\n");
		pthread_exit(NULL);
	}

	double elapsed_time = g_timer_elapsed(f->timers[i], NULL);

	while ((elapsed_time * 1000) < f->max_duration) {
		elapsed_time = g_timer_elapsed(f->timers[i], NULL);
	}
	g_timer_stop(f->timers[i]);

	queue_delete(f->q, f);

	pthread_exit(NULL);
}
void firework_make_thread(Firework *f) {
	for (int i = 0; i < f->num_of_angles; i++) {
		pthread_t tid;
		struct monitorargs *arg =
		    (struct monitorargs *)malloc(sizeof(struct monitorargs));
		arg->f = f;
		arg->i = i;
		pthread_create(&tid, NULL, monitor_timer, (void *)arg);
	}
}

// int main() {
// 	srand(time(NULL));
//
// 	Firework fireworks[4];
//
// 	for (int i = 0; i < 4; i++) {
// 		firework_new(&fireworks[i], rand() % 100, rand() % 100);
//
// 		firework_set_rgb(&fireworks[i], (double)(rand() % 255) / 255.0,
// 		                 (double)(rand() % 255) / 255.0,
// 		                 (double)(rand() % 255) / 255.0);
// 		printf(
// 		    "New firework at (%d %d) with color (%.2f, %.2f, %.2f) "
// 		    "with %d angles\n",
// 		    fireworks[i].x, fireworks[i].y, fireworks[i].r,
// 		    fireworks[i].g, fireworks[i].b, fireworks[i].num_of_angles);
// 	}
// }
