#include <gtk/gtk.h>
#include <stdio.h>

static int pi = 0;
static int brush_size = 4;

static int points[100][2];

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

	for (int i = 0; i < pi; i++) {
		GdkRectangle drawRect;
		int cx = points[i][0], cy = points[i][1];
		drawRect.x = cx - brush_size;
		drawRect.y = cy - brush_size;
		drawRect.width = brush_size*2;
		drawRect.height = brush_size*2;

		cairo_set_source_rgb(cr, 1, 0, 1);
		gdk_cairo_rectangle(cr, &drawRect);
		cairo_fill(cr);
	}
}

void open_dialog_response(GtkNativeDialog *dialog, int response) {
	if (response == GTK_RESPONSE_ACCEPT) {
		char *path = g_file_get_path(
		    gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog)));
		printf("Response %d: accepted, got path %s\n", response, path);
	} else if (response == GTK_RESPONSE_CANCEL) {
		printf("Response %d: cancelled\n", response);
	}
}

void open_clicked(GtkWidget *button, GtkWindow *parent) {
	GtkFileChooserNative *dialog = gtk_file_chooser_native_new(
	    "Open picture", GTK_WINDOW(parent), GTK_FILE_CHOOSER_ACTION_OPEN,
	    "Open", "Cancel");

	GtkFileFilter *png_filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(png_filter, "*.png");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), png_filter);

	gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));

	g_signal_connect(dialog, "response", G_CALLBACK(open_dialog_response),
	                 NULL);
}

void canvas_clicked(GtkGestureClick *gesture, int n, double x, double y,
                    GtkWidget *canvas) {
	points[pi][0] = (int)x;
	points[pi][1] = (int)y;
	pi++;
	gtk_widget_queue_draw(canvas);
	printf("added %.2f %.2f to list\n", x, y);
}

GtkWidget *make_toolbar(GtkWidget *parent) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

	gtk_widget_set_halign(GTK_WIDGET(box), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(box), GTK_ALIGN_CENTER);

	GtkWidget *openButton = gtk_button_new_with_label("Open");

	g_signal_connect(openButton, "clicked", G_CALLBACK(open_clicked),
	                 parent);

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

	// ------- MOUSE GESTURE -------
	GtkGesture *clickGesture = gtk_gesture_click_new();
	gtk_widget_add_controller(canvas, GTK_EVENT_CONTROLLER(clickGesture));
	g_signal_connect(clickGesture, "pressed", G_CALLBACK(canvas_clicked),
	                 canvas);

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
