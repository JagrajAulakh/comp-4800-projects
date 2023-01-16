#include <gtk/gtk.h>
#include <stdio.h>

int dropdown_index = 0;

void setMargin(GtkWidget *widget, int margin) {
	gtk_widget_set_margin_end(widget, margin);
	gtk_widget_set_margin_start(widget, margin);
	gtk_widget_set_margin_top(widget, margin);
	gtk_widget_set_margin_bottom(widget, margin);
}

void spawnInfoWindow(GtkApplication *app, const char *message) {
	GtkWidget *infoWindow, *mainBox, *buttonBox, *infoMessage, *okButton;
	infoWindow = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(infoWindow), "Info window");

	mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
	gtk_widget_set_valign(mainBox, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(mainBox, GTK_ALIGN_CENTER);
	setMargin(mainBox, 20);

	buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_widget_set_valign(buttonBox, GTK_ALIGN_END);
	gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);

	infoMessage = gtk_label_new(message);

	okButton = gtk_button_new_with_label("OK");
	g_signal_connect_swapped(okButton, "clicked", G_CALLBACK(gtk_window_destroy), infoWindow);

	gtk_window_set_child(GTK_WINDOW(infoWindow), mainBox);
	gtk_box_append(GTK_BOX(mainBox), infoMessage);
	gtk_box_append(GTK_BOX(mainBox), buttonBox);
	gtk_box_append(GTK_BOX(buttonBox), okButton);

	gtk_widget_show(infoWindow);
}

// Button clicked callback function
void click_callback(GtkApplication *app, GtkWidget *widget) {

	switch (dropdown_index) {
		case 0:
			spawnInfoWindow(app, "This is an info box");
			break;
	}
}

// Dropdown change callback function
void dropdown_callback(GtkWidget *widget, gpointer data) {
	dropdown_index = gtk_drop_down_get_selected(GTK_DROP_DOWN(widget));
}

void activate(GtkApplication *app, gpointer user_data) {
	GtkWidget *window, *button, *main_box, *dropdown_box, *dropdown_label, *dropdown;

	// Make main application window
	window = gtk_application_window_new(app);
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
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(click_callback), G_APPLICATION(app));
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
	GtkApplication *app;
	int status;

	app = gtk_application_new("org.bigbois.example",
	                          G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);

	return status;
}
