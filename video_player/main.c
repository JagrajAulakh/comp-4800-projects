#include <gtk/gtk.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>

static GtkWidget *drawingArea;
static GTimer *timer;
static cairo_surface_t *image1, *image2;

const unsigned long duration = 800;

void setWidgetCss(GtkWidget *widget, const char *css) {
	GtkCssProvider *provider = gtk_css_provider_new();
	const char *data = css;
	gtk_css_provider_load_from_data(provider, data, -1);
	GdkDisplay *displej = gtk_widget_get_display(GTK_WIDGET(widget));
	gtk_style_context_add_provider_for_display(
	    displej, GTK_STYLE_PROVIDER(provider),
	    GTK_STYLE_PROVIDER_PRIORITY_USER);
}

void draw(GtkDrawingArea *drawingArea, cairo_t *cr, int width, int height,
          gpointer data) {
	if (image1 != NULL && image2 != NULL) {
		cairo_set_source_surface(cr, image1, 0, 0);
		cairo_paint(cr);

		static double percentage = 0.0, smooth_percentage1 = 0.0,
		       smooth_percentage2 = 0.0;
		if (g_timer_is_active(timer)) {
			percentage =
			    g_timer_elapsed(timer, NULL) * 1000 / duration;
			smooth_percentage1 =
			    (1 - cos(percentage * M_PI)) / 2;          // cosine
			smooth_percentage2 = percentage * percentage;  // x^2
		}

		int total_width = cairo_image_surface_get_width(image1);
		int total_height = cairo_image_surface_get_height(image1);
		const int boxes_x = 5;
		const int boxes_y = 2;

		int stride_x = total_width / boxes_x;
		int stride_y = total_height / boxes_y;

		cairo_set_source_surface(cr, image2, 0, 0);
		for (int bx = 0; bx < boxes_x; bx++) {
			for (int by = 0; by < boxes_y; by++) {
				cairo_reset_clip(cr);
				int px = stride_x * percentage;
				int py = stride_y * percentage;
				cairo_rectangle(cr,
				bx*stride_x+stride_x/2-px/2,
				by*stride_y+stride_y/2-py/2, px, py);
				// cairo_arc(cr, bx * stride_x + stride_x / 2,
				//           by * stride_y + stride_y / 2,
				//           (px < py ? px : py)/2, 0, 2 * M_PI);
				cairo_clip(cr);
				cairo_paint(cr);
			}
		}
	}
}

void *drawing_loop(void *d) {
	unsigned long duration = *((unsigned long *)d);
	double elapsed_time = 0;

	if (timer == NULL) {
		fprintf(stderr, "Timer is null\n");
		pthread_exit(NULL);
	}

	while ((elapsed_time * 1000) < duration) {
		elapsed_time = g_timer_elapsed(timer, NULL);
	}
	g_timer_stop(timer);

	pthread_exit(NULL);
}

void start_timer_click_callback(GtkWidget *button, gpointer data) {
	if (g_timer_is_active(timer)) {
		return;
	}

	g_timer_reset(timer);
	g_timer_start(timer);

	pthread_t drawing_thread;

	pthread_create(&drawing_thread, NULL, drawing_loop, (void *)&duration);
	// pthread_join(drawing_thread, NULL);
}

GtkWidget *make_toolbar(GtkWidget *parent) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

	gtk_widget_set_halign(GTK_WIDGET(box), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(box), GTK_ALIGN_CENTER);

	GtkWidget *startTimerButton;

	startTimerButton = gtk_button_new_with_label("Start timer");
	g_signal_connect(startTimerButton, "clicked",
	                 G_CALLBACK(start_timer_click_callback), NULL);

	gtk_box_append(GTK_BOX(box), startTimerButton);

	return box;
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

	// ------- TOOLBAR -------
	GtkWidget *toolbar = make_toolbar(window);
	gtk_box_append(GTK_BOX(box), toolbar);

	// ------- CANVAS -------
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawingArea), draw,
	                               NULL, NULL);
	gtk_box_append(GTK_BOX(box), drawingArea);

	// ------- LOAD IMAGES -------
	image1 = cairo_image_surface_create_from_png("./assets/wallpaper1.png");
	image2 = cairo_image_surface_create_from_png("./assets/wallpaper2.png");
	gtk_widget_set_size_request(drawingArea,
	                            cairo_image_surface_get_width(image1),
	                            cairo_image_surface_get_height(image1));

	// ------- MAKE TIMER -------
	timer = g_timer_new();
	g_timer_stop(timer);
	g_timer_reset(timer);

	g_timeout_add(1000 / 60, (GSourceFunc)gtk_widget_queue_draw,
	              drawingArea);
	gtk_widget_queue_draw(drawingArea);
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
