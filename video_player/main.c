#include <gtk/gtk.h>
#include <math.h>
#include <stdio.h>

// "Points index", not Math.PI lmao
static int pi = 0;
static int pi_max = 0;
static int points[100][3];
static GdkRGBA point_colors[100], selected_color;

static int brush_size = 1;

static cairo_surface_t *loaded_image = NULL, *canvas = NULL;
GtkWidget *drawingArea = NULL, *colorSquare = NULL;

enum { CLICK_OPERATION_GET_COLOR, CLICK_OPERATION_PAINT };
static int click_operation = CLICK_OPERATION_GET_COLOR;

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
	GtkStyleContext *drawingAreaContext;

	// Get the style context of canvas. The style context let's us draw (I
	// think)
	drawingAreaContext =
	    gtk_widget_get_style_context(GTK_WIDGET(drawingArea));
	cairo_t *canvasCairo = cairo_create(canvas);

	if (loaded_image != NULL) {
		int image_width = cairo_image_surface_get_width(loaded_image),
		    image_height = cairo_image_surface_get_height(loaded_image);

		cairo_set_source_surface(canvasCairo, loaded_image, 0, 0);
		cairo_paint(canvasCairo);

		for (int i = 0; i < pi; i++) {
			int cx = points[i][0], cy = points[i][1],
			    brush_size = points[i][2];

			GdkRGBA color = point_colors[i];
			cairo_set_source_rgb(canvasCairo, color.red,
			                     color.green, color.blue);
			cairo_arc(canvasCairo, cx, cy, brush_size, 0, 2 * M_PI);
			cairo_fill(canvasCairo);
		}

		cairo_set_source_surface(cr, canvas, 0, 0);
		cairo_paint(cr);
	}
}

int save_image(const char *path) {
	cairo_status_t status = cairo_surface_write_to_png(canvas, path);
	if (status == CAIRO_STATUS_SUCCESS) {
		return 1;
	}
	return 0;
}
void load_image(const char *path) {
	loaded_image = cairo_image_surface_create_from_png(path);
	canvas = cairo_image_surface_create(
	    cairo_image_surface_get_format(loaded_image),
	    cairo_image_surface_get_width(loaded_image),
	    cairo_image_surface_get_height(loaded_image));

	gtk_widget_set_size_request(
	    GTK_WIDGET(drawingArea),
	    cairo_image_surface_get_width(loaded_image),
	    cairo_image_surface_get_height(loaded_image));
	pi = 0;
	pi_max = 0;
}

void open_dialog_response(GtkNativeDialog *dialog, int response) {
	if (response == GTK_RESPONSE_ACCEPT) {
		const char *path = g_file_get_path(
		    gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog)));
		load_image(path);
		gtk_widget_queue_draw(drawingArea);
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

void save_dialog_response(GtkNativeDialog *dialog, int response) {
	if (response == GTK_RESPONSE_ACCEPT) {
		const char *path = g_file_get_path(
		    gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog)));
		save_image(path);
	}
}

void save_clicked(GtkWidget *button, GtkWindow *parent) {
	GtkFileChooserNative *dialog = gtk_file_chooser_native_new(
	    "Save png", parent, GTK_FILE_CHOOSER_ACTION_SAVE, "Save", "Cancel");
	GtkFileFilter *png_filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(png_filter, "*.png");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), png_filter);

	gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));

	g_signal_connect(dialog, "response", G_CALLBACK(save_dialog_response),
	                 NULL);
}

void undo_clicked(GtkWidget *button, GtkWidget *drawingArea) {
	pi--;
	if (pi < 0) {
		pi = 0;
	}
	gtk_widget_queue_draw(drawingArea);
}
void redo_clicked(GtkWidget *button, GtkWidget *drawingArea) {
	pi++;
	if (pi > pi_max) {
		pi = pi_max;
	}
	gtk_widget_queue_draw(drawingArea);
}

void brush_spin_button_change(GtkSpinButton *spin, gpointer data) {
	int val = gtk_spin_button_get_value_as_int(spin);
	brush_size = val;
}

void canvas_clicked(GtkGestureClick *gesture, int n, double x, double y,
                    GtkWidget *drawingArea) {
	if (click_operation == CLICK_OPERATION_PAINT) {
		points[pi][0] = (int)x;
		points[pi][1] = (int)y;
		points[pi][2] = brush_size;
		point_colors[pi].red = selected_color.red;
		point_colors[pi].green = selected_color.green;
		point_colors[pi].blue = selected_color.blue;

		pi++;
		pi_max = pi;
		gtk_widget_queue_draw(drawingArea);
	} else if (click_operation == CLICK_OPERATION_GET_COLOR) {
		cairo_surface_flush(loaded_image);
		unsigned char *image_data =
		    cairo_image_surface_get_data(loaded_image);

		int image_width = cairo_image_surface_get_width(loaded_image);
		int image_height = cairo_image_surface_get_height(loaded_image);

		int stride = cairo_image_surface_get_stride(loaded_image);
		unsigned char *pixel =
		    image_data + (int)y * stride + (int)x * 4;
		unsigned char r = pixel[2];
		unsigned char g = pixel[1];
		unsigned char b = pixel[0];
		selected_color.red = (double)r / 255.0;
		selected_color.green = (double)g / 255.0;
		selected_color.blue = (double)b / 255.0;

		char css[200];
		sprintf(css,
		        "button#color-square { background-color: rgb(%d, %d, "
		        "%d); }",
		        r, g, b);
		setWidgetCss(colorSquare, css);
	}
}

void mode_switch_callback(GtkSwitch *widget, gboolean newState,
                          GtkWidget *label) {
	gtk_switch_set_state(GTK_SWITCH(widget), newState);
	bool state = gtk_switch_get_state(widget);
	if (state) {
		gtk_label_set_label(GTK_LABEL(label), "Paint");
		click_operation = CLICK_OPERATION_PAINT;
	} else {
		gtk_label_set_label(GTK_LABEL(label), "Get Color");
		click_operation = CLICK_OPERATION_GET_COLOR;
	}
}

GtkWidget *make_toolbar(GtkWidget *parent) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

	gtk_widget_set_halign(GTK_WIDGET(box), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(box), GTK_ALIGN_CENTER);

	GtkWidget *openButton = gtk_button_new_with_label("Open"),
	          *saveButton = gtk_button_new_with_label("Save"),
	          *undoButton = gtk_button_new_with_label("Undo"),
	          *redoButton = gtk_button_new_with_label("Redo");

	GtkWidget *modeSwitchLabel = gtk_label_new("Get Color"),
	          *modeSwitch = gtk_switch_new();
	GtkWidget *modeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_switch_set_active(GTK_SWITCH(modeSwitch), true);
	gtk_switch_set_state(GTK_SWITCH(modeSwitch), false);
	g_signal_connect(modeSwitch, "state-set",
	                 G_CALLBACK(mode_switch_callback), modeSwitchLabel);
	gtk_box_append(GTK_BOX(modeBox), modeSwitchLabel);
	gtk_box_append(GTK_BOX(modeBox), modeSwitch);

	GtkWidget *brushSizeSpinButton =
	              gtk_spin_button_new_with_range(1, 20, 1),
	          *brushSizeLabel = gtk_label_new("Brush size:");
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(brushSizeSpinButton), 0);

	colorSquare = gtk_button_new();
	gtk_widget_set_name(colorSquare, "color-square");

	g_signal_connect(openButton, "clicked", G_CALLBACK(open_clicked),
	                 parent);
	g_signal_connect(saveButton, "clicked", G_CALLBACK(save_clicked),
	                 parent);
	g_signal_connect(undoButton, "clicked", G_CALLBACK(undo_clicked),
	                 drawingArea);
	g_signal_connect(redoButton, "clicked", G_CALLBACK(redo_clicked),
	                 drawingArea);
	g_signal_connect(brushSizeSpinButton, "value-changed",
	                 G_CALLBACK(brush_spin_button_change), NULL);

	gtk_box_append(GTK_BOX(box), openButton);
	gtk_box_append(GTK_BOX(box), saveButton);
	gtk_box_append(GTK_BOX(box), undoButton);
	gtk_box_append(GTK_BOX(box), redoButton);
	gtk_box_append(GTK_BOX(box), modeBox);
	gtk_box_append(GTK_BOX(box), colorSquare);
	gtk_box_append(GTK_BOX(box), brushSizeLabel);
	gtk_box_append(GTK_BOX(box), brushSizeSpinButton);

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
	load_image("./demo.png");

	// ------- MOUSE GESTURE -------
	GtkGesture *clickGesture = gtk_gesture_click_new();
	gtk_widget_add_controller(drawingArea,
	                          GTK_EVENT_CONTROLLER(clickGesture));
	g_signal_connect(clickGesture, "pressed", G_CALLBACK(canvas_clicked),
	                 drawingArea);

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
