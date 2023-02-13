#include <cairo.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>



cairo_surface_t *canvas;
GtkWidget *area;
GTimer *timer;

//draw function which paints and draws the red lines for get color
static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
  double elapsed_time = g_timer_elapsed(timer, NULL);
  canvas = cairo_get_target(cr);
  cairo_surface_t *pic = cairo_image_surface_create_from_png("space.png");
  cairo_scale(cr,0.5, 0.5);
  gtk_widget_set_size_request(GTK_WIDGET(area),cairo_image_surface_get_width(pic)/2,cairo_image_surface_get_height(pic)/2);
  cairo_set_source_surface(cr, pic, 0, 0);
  cairo_paint(cr);
  cairo_surface_destroy(pic);
  cairo_surface_t *rocketship = cairo_image_surface_create_from_png("spaceship2.png");
  cairo_surface_set_device_scale(rocketship, 0.4, 0.4);
  if(elapsed_time <= 4){
  	cairo_set_source_surface(cr, rocketship, 850, 600 - elapsed_time*150);
  	cairo_paint(cr);
  }
  else{
  	cairo_set_source_surface(cr, rocketship, 850, (elapsed_time-4)*150);
  	cairo_paint(cr);
  }
  if(elapsed_time >8){
  	g_timer_reset(timer);
  }
  cairo_surface_destroy(rocketship);
}

void activate(GtkApplication *app, gpointer user_data) {
	timer = g_timer_new();
	GtkWidget *window,*box;
	window = gtk_application_window_new(app);
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
	gtk_window_set_child(GTK_WINDOW(window), box);
	gtk_window_set_title(GTK_WINDOW(window), "rocket demo");
	gtk_widget_show(window);
	area = gtk_drawing_area_new();
	gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(area), 500);
	gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), 400);
  	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_function, NULL, NULL);
  	gtk_widget_set_size_request(GTK_WIDGET(area), 300, 400);
  	gtk_box_append(GTK_BOX(box), area);
  	g_timeout_add(1000 / 30, (GSourceFunc)gtk_widget_queue_draw, area);
}


int main(int argc, char *argv[]) {
	GtkApplication *app;
	int status;

	app = gtk_application_new("animation-demo.org",
	                          G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}