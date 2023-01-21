#include <gtk/gtk.h>
#include <stdio.h>

void activate(GtkApplication *app, gpointer user_data) {
	GtkWidget *window, *box;

	window = gtk_application_window_new(app);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

	gtk_window_set_child(GTK_WINDOW(window), box);

	// gtk_box_append(GTK_BOX(box), button);

	gtk_window_set_title(GTK_WINDOW(window), "Main application window");
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

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
