#include <gtk/gtk.h>
#include <stdio.h>

// Button clicked callback function
void click_callback(GtkWidget *widget, gpointer data) {
	g_print("Hello World!\n");
}

// Dropdown change callback function
void dropdown_callback(GtkWidget *widget, gpointer data) {
	printf("%d\n", gtk_drop_down_get_selected(GTK_DROP_DOWN(widget)));
}

void activate(GtkApplication *app, gpointer user_data) {
	GtkWidget *window, *button, *box, *dropdown;

	// Make main application window
	window = gtk_application_window_new(app);
	// Make a button with text
	button = gtk_button_new_with_label("Hello World");
	// Make a box that will hold the button
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	// Make a dropdown menu with predefined values
	dropdown = gtk_drop_down_new_from_strings(
	    (const char *[]){"joe", "mama", NULL});

	// Set box alignment to centered
	gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

	// Define callback function for button click
	g_signal_connect(button, "clicked", G_CALLBACK(click_callback), NULL);
	// Define callback function for button click, but pass custom arguments
	// to callback function
	g_signal_connect_swapped(button, "clicked",
	                         G_CALLBACK(gtk_window_destroy), window);
	// Call function everytime user changes dropdown value
	g_signal_connect(dropdown, "notify::selected-item",
	                 G_CALLBACK(dropdown_callback), NULL);

	// Main window displays the box
	gtk_window_set_child(GTK_WINDOW(window), box);
	// Add button to box
	gtk_box_append(GTK_BOX(box), button);
	// Add dropdown to box
	gtk_box_append(GTK_BOX(box), dropdown);

	// Set window title
	gtk_window_set_title(GTK_WINDOW(window), "Main application window");
	// Define window size
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

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
