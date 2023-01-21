#include <gtk/gtk.h>
#include <stdio.h>

void draw(GtkDrawingArea *canvas, cairo_t *cr, int width, int height,
          gpointer data) {
	GtkStyleContext *context;

	// Get the style context of canvas. The style context let's us draw (I
	// think)
	context = gtk_widget_get_style_context(GTK_WIDGET(canvas));

	cairo_surface_t *demo_image =
	    cairo_image_surface_create_from_png("./demo.png");
	int demo_width = cairo_image_surface_get_width(demo_image),
	    demo_height = cairo_image_surface_get_height(demo_image);

	cairo_set_source_surface(cr, demo_image, 0, 0);
	cairo_paint(cr);
}


GtkWidget *make_toolbar(GtkWidget *parent) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

	gtk_widget_set_halign(GTK_WIDGET(box), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(box), GTK_ALIGN_CENTER);

	GtkWidget *openButton = gtk_button_new_with_label("Open");

	gtk_box_append(GTK_BOX(box), openButton);

	return box;
}

void activate(GtkApplication *app, gpointer user_data) {
	GtkWidget *window, *box, *canvas;

	window = gtk_application_window_new(app);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	canvas = gtk_drawing_area_new();

	// ------- WINDOW -------
	gtk_window_set_title(GTK_WINDOW(window), "Main application window");
	// ------- BOX -------
	gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
	gtk_window_set_child(GTK_WINDOW(window), box);

	// ------- TOOLBAR -------
	GtkWidget *toolbar = make_toolbar(window);
	gtk_box_append(GTK_BOX(box), toolbar);

	// ------- CANVAS -------
	gtk_widget_set_size_request(canvas, 800, 600);
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(canvas), draw, NULL,
	                               NULL);
	gtk_box_append(GTK_BOX(box), canvas);

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
