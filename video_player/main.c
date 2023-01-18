#include <gtk/gtk.h>
#include <stdio.h>

int dropdown_index = 0;

void setMargin(GtkWidget *widget, int margin) {
	gtk_widget_set_margin_end(widget, margin);
	gtk_widget_set_margin_start(widget, margin);
	gtk_widget_set_margin_top(widget, margin);
	gtk_widget_set_margin_bottom(widget, margin);
}

void setWidgetCss(GtkWidget *widget, const char *css) {
	GtkCssProvider *provider = gtk_css_provider_new();
	const char *data = css;
	gtk_css_provider_load_from_data(provider, data, -1);
	GdkDisplay *displej = gtk_widget_get_display(GTK_WIDGET(widget));
	gtk_style_context_add_provider_for_display(
	    displej, GTK_STYLE_PROVIDER(provider),
	    GTK_STYLE_PROVIDER_PRIORITY_USER);
}

GtkWidget *makeInfoWindow(GtkApplication *app, const char *message) {
	GtkWidget *infoWindow, *mainBox, *buttonBox, *infoMessage, *okButton;
	infoWindow = gtk_application_window_new(app);

	mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
	gtk_widget_set_valign(mainBox, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(mainBox, GTK_ALIGN_CENTER);
	setMargin(mainBox, 20);

	buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_widget_set_valign(buttonBox, GTK_ALIGN_END);
	gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);

	infoMessage = gtk_label_new(message);

	okButton = gtk_button_new_with_label("OK");
	g_signal_connect_swapped(okButton, "clicked",
	                         G_CALLBACK(gtk_window_destroy), infoWindow);

	gtk_window_set_child(GTK_WINDOW(infoWindow), mainBox);
	gtk_box_append(GTK_BOX(mainBox), infoMessage);
	gtk_box_append(GTK_BOX(mainBox), buttonBox);
	gtk_box_append(GTK_BOX(buttonBox), okButton);

	return infoWindow;
}

void spawnInfoWindow(GtkApplication *app, const char *message) {
	GtkWidget *infoWindow = makeInfoWindow(app, message);
	gtk_window_set_title(GTK_WINDOW(infoWindow), "Info window");
	gtk_widget_show(infoWindow);
}

void spawnWarningWindow(GtkApplication *app, const char *message) {
	GtkWidget *warningWindow = makeInfoWindow(app, message);
	GtkWidget *child = gtk_window_get_child(GTK_WINDOW(warningWindow));

	gtk_window_set_title(GTK_WINDOW(warningWindow), "Warning window");

	gtk_widget_set_name(warningWindow, "warningWindow");

	setWidgetCss(warningWindow, "window#warningWindow {background: red;}");
	gtk_widget_show(warningWindow);
}

void randomBackground(GtkWidget *widget, GtkWidget *window) {
	int r = rand() % 255, g = rand() % 255, b = rand() % 255;
	char cssProp[100];
	sprintf(cssProp, "window#parent {background: rgb(%d, %d, %d);}", r, g,
	        b);
	setWidgetCss(window, cssProp);
}

GtkWidget *makeColorChangeWindow(GtkApplication *app) {
	GtkWindow *parentWindow = gtk_application_get_active_window(app);

	printf("Parent window has title: %s\n",
	       gtk_window_get_title(GTK_WINDOW(parentWindow)));

	GtkWidget *window, *button, *box;
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Random Color");

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	setMargin(box, 20);

	button = gtk_button_new_with_label("Random color");

	g_signal_connect(button, "clicked", G_CALLBACK(randomBackground), parentWindow);

	gtk_window_set_child(GTK_WINDOW(window), box);
	gtk_box_append(GTK_BOX(box), button);

	return window;
}

void spawnColorWindow(GtkApplication *app) {
	GtkWidget *window = makeColorChangeWindow(app);
	gtk_widget_show(window);
}

// Button clicked callback function
void click_callback(GtkApplication *app, GtkWidget *widget) {
	switch (dropdown_index) {
		case 0:
			spawnInfoWindow(app, "This is an info window");
			break;
		case 1:
			spawnWarningWindow(app, "WARNING: This is a serious warning!");
			break;
		case 2:
			spawnColorWindow(app);
			break;
	}
}

// Dropdown change callback function
void dropdown_callback(GtkWidget *widget, gpointer data) {
	dropdown_index = gtk_drop_down_get_selected(GTK_DROP_DOWN(widget));
}

void activate(GtkApplication *app, gpointer user_data) {
	GtkWidget *window, *button, *main_box, *dropdown_box, *dropdown_label,
	    *dropdown;

	// Make main application window
	window = gtk_application_window_new(app);
	gtk_widget_set_name(window, "parent");
	// Make a button with text
	button = gtk_button_new_with_label("Submit");
	// Make a box that will hold the button
	main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 30);

	dropdown_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	// Make a dropdown menu with predefined values
	dropdown = gtk_drop_down_new_from_strings(
	    (const char *[]){"info", "warning", "color change window", NULL});
	dropdown_label = gtk_label_new("Function: ");

	// Set box alignment to centered
	gtk_widget_set_halign(main_box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(main_box, GTK_ALIGN_CENTER);

	// Define callback function for button click
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(click_callback),
	                         G_APPLICATION(app));
	// Call function everytime user changes dropdown value
	g_signal_connect(dropdown, "notify::selected-item",
	                 G_CALLBACK(dropdown_callback), NULL);

	// Main window displays the box
	gtk_window_set_child(GTK_WINDOW(window), main_box);
	// Add dropdown to box
	gtk_box_append(GTK_BOX(dropdown_box), dropdown_label);
	gtk_box_append(GTK_BOX(dropdown_box), dropdown);
	// Add button to box
	gtk_box_append(GTK_BOX(main_box), dropdown_box);
	gtk_box_append(GTK_BOX(main_box), button);

	// Set window title
	gtk_window_set_title(GTK_WINDOW(window), "Parent window");
	// Define window size
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

	// Show main window
	gtk_widget_show(window);
}

int main(int argc, char *argv[]) {
	srand(time(NULL));

	GtkApplication *app;
	int status;

	app = gtk_application_new("org.bigbois.example",
	                          G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);

	return status;
}
