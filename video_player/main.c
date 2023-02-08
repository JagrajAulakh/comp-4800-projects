#include <gtk/gtk.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>


typedef struct {
	int x, y;
} Eye;

static GtkWidget *drawingArea;
static cairo_surface_t *canvas, *backgroundImage;

static int mx, my;
static Eye eyes[4];

double getAngle(double x1, double y1, double x2, double y2) {
	return atan((y2-y1)/(x2-x1));
}

void draw(GtkDrawingArea *drawingArea, cairo_t *cr, int width, int height,
          gpointer data) {
	cairo_set_source_surface(cr, backgroundImage, 0, 0);
	cairo_paint(cr);

	cairo_set_source_rgb(cr, 1, 0, 1);
	// Draw eyes
	for (int i = 0; i < 2; i++) {
		Eye eye = eyes[i];
		cairo_arc(cr, eye.x, eye.y, 30, 0, 2 * M_PI);
		cairo_fill(cr);
	}
}

// void *drawing_loop(void *d) {
// 	unsigned long duration = *((unsigned long *)d);
// 	double elapsed_time = 0;
//
// 	if (timer == NULL) {
// 		fprintf(stderr, "Timer is null\n");
// 		pthread_exit(NULL);
// 	}
//
// 	while ((elapsed_time * 1000) < duration) {
// 		elapsed_time = g_timer_elapsed(timer, NULL);
// 	}
// 	g_timer_stop(timer);
//
// 	pthread_exit(NULL);
// }

void mouse_motion(GtkWidget widget, double x, double y, gpointer data) {
	mx = (int)x;
	my = (int)y;
}

void activate(GtkApplication *app, gpointer user_data) {
	GtkWidget *window, *box;

	window = gtk_application_window_new(app);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	drawingArea = gtk_drawing_area_new();

	// ------- WINDOW -------
	gtk_window_set_title(GTK_WINDOW(window), "Main application window");
	// ------- BOX -------
	gtk_widget_set_halign(box, GTK_ALIGN_START);
	gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
	gtk_window_set_child(GTK_WINDOW(window), box);

	// ------- CANVAS -------
	canvas = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 800, 600);
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawingArea), draw,
	                               NULL, NULL);
	gtk_box_append(GTK_BOX(box), drawingArea);

	// ------- LOAD IMAGES -------
	backgroundImage =
	    cairo_image_surface_create_from_png("./assets/rickmorty_blue.png");

	gtk_widget_set_size_request(
	    drawingArea, cairo_image_surface_get_width(backgroundImage),
	    cairo_image_surface_get_height(backgroundImage));

	// ------- MOUSE GESTURE -------
	GtkEventController *mouseMotionController =
	    gtk_event_controller_motion_new();
	gtk_widget_add_controller(window, mouseMotionController);
	g_signal_connect(mouseMotionController, "motion",
	                 G_CALLBACK(mouse_motion), NULL);

	// ------- EYES -------
	eyes[0].x = 627;
	eyes[0].y = 383;
	eyes[1].x = 700;
	eyes[1].y = 383;

	gtk_widget_show(window);
}

int main(int argc, char *argv[]) {
	GtkApplication *app;
	int status;

	app = gtk_application_new("org.bigbois.example",
	                          G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);

	return status;
}
