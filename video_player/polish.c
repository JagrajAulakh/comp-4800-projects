#include <cairo.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>



double x_global[100];
double y_global[100];
double x_point;
double y_point; //used for red lines middle point
int counter = 0;
bool switcher = true;
float r[100];
float g[100];
float b[100];
float r1;
float g1;
float b1;
int max_counter = 0;
cairo_surface_t *canvas;
GtkWidget *area;


void css(GtkWidget *widget, const char *css) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *data = css;
    gtk_css_provider_load_from_data(provider, data, -1);
    GdkDisplay *displej = gtk_widget_get_display(GTK_WIDGET(widget));
    gtk_style_context_add_provider_for_display(
        displej, GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
}

void dialog_callback(GtkNativeDialog *filec, int response ){ 
	cairo_status_t status;
  	if (response == GTK_RESPONSE_ACCEPT)
  	{
  		const char *filename = g_file_get_path(gtk_file_chooser_get_file(GTK_FILE_CHOOSER(filec)));
    	status = cairo_surface_write_to_png(canvas, filename);
  	}

}

//creates a save button
void savebutton_c(GtkWidget *widget)
{
  GtkFileChooserNative *dialog;
  dialog = gtk_file_chooser_native_new("Save File", NULL,GTK_FILE_CHOOSER_ACTION_SAVE, "SAVE", "CANCEL");
  gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
  g_signal_connect(dialog, "response", G_CALLBACK(dialog_callback), NULL);
}



//gets the current pixel and sets color on toolbar and calculates
void getpixel(GtkGestureClick *gest, int n, double x_axis, double y_axis,GtkWidget *colorbutton){
	int row;
	int i;
	if(!switcher){	
	   unsigned char* rgba;
	   cairo_surface_t *pic = cairo_image_surface_create_from_png("icon.png");
	   gtk_widget_set_size_request(GTK_WIDGET(area),cairo_image_surface_get_width(pic),cairo_image_surface_get_height(pic));
	   int stride = cairo_image_surface_get_stride(pic);
	   row = stride * y_axis;
	   i = row + (4 * x_axis);
	   rgba = cairo_image_surface_get_data(pic);
	   b1 = (float)rgba[i]/255;
	   g1 = (float)rgba[i+1]/255;
	   r1 = (float)rgba[i+2]/255;
	   char arr[100];
	   sprintf(arr, "button#colorbutt {background:rgb(%f,%f,%f);}",r1*255, g1*255, b1*255);
	   css(colorbutton,arr);
	   gtk_widget_queue_draw(area);
	 }
}

//draw function which paints and draws the red lines for get color
static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
  canvas = cairo_get_target(cr);
  cairo_surface_t *pic = cairo_image_surface_create_from_png("icon.png");
  gtk_widget_set_size_request(GTK_WIDGET(area),cairo_image_surface_get_width(pic),cairo_image_surface_get_height(pic));
  cairo_set_source_surface(cr, pic, 0, 0);
  cairo_paint(cr);
  cairo_surface_destroy(pic);
  for(int i = 0; i<counter; i++){
  	cairo_arc(cr, x_global[i], y_global[i], 5, 0, 2 * M_PI);
  	cairo_set_source_rgb(cr, r[i],g[i],b[i]);
  	cairo_fill (cr);
  }
  if(!switcher){
  	cairo_move_to(cr, x_point, 0);
    cairo_line_to(cr, x_point, 800);
    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_stroke(cr);
    cairo_move_to(cr, 0, y_point);
    cairo_line_to(cr, 1000, y_point);
    cairo_stroke(cr);
  }
}


//gets the x and y axis on location clicked
void clicked(GtkGestureClick *gest, int n, double x_axis, double y_axis, GtkWidget *area){

	if(switcher){
	x_global[counter] = x_axis;
	y_global[counter] = y_axis;
	r[counter] = r1;
	b[counter] = b1;
	g[counter] = g1;
	counter++;
	max_counter = counter;
	gtk_widget_queue_draw(area);
	}
	else if(!switcher){
	x_point = x_axis;
	y_point = y_axis;

	}
}

//sets if paint pressed
void paint_area_button(GtkWidget *widget, gpointer data) {
	switcher = true;
}

//sets if getcolor pressed
void get_color_button(GtkWidget *widget, gpointer data){
	switcher = false;
}


void backward_button(GtkWidget *widget, GtkWidget *area){
	if(counter>0){
		counter--;
		gtk_widget_queue_draw(area);
	}
}

//forward button
void forward_button(GtkWidget *widget, GtkWidget *area){
	if(max_counter>counter){
		counter++;
	}
	gtk_widget_queue_draw(area);
}

void activate(GtkApplication *app, gpointer user_data) {
	GtkWidget *window, *button_paint, *button_getcolor, *button_backward, *button_forward, *box, *colorbutton, *savebutton;
	window = gtk_application_window_new(app);
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	button_paint = gtk_button_new_with_label("Paint");
	button_getcolor = gtk_button_new_with_label("getcolor");
	button_backward = gtk_button_new_with_label("backward");
	button_forward = gtk_button_new_with_label("forward");
	savebutton = gtk_button_new_with_label("save");
	colorbutton = gtk_button_new();
	gtk_widget_set_name(colorbutton, "colorbutt");
	gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
	gtk_window_set_child(GTK_WINDOW(window), box);
	gtk_window_set_title(GTK_WINDOW(window), "painter window");
	gtk_widget_set_halign(button_paint, GTK_ALIGN_START);
	gtk_widget_set_valign(button_paint, GTK_ALIGN_START);
	gtk_widget_set_halign(button_getcolor, GTK_ALIGN_START);
	gtk_widget_set_valign(button_getcolor, GTK_ALIGN_START);
	gtk_widget_set_halign(button_backward, GTK_ALIGN_START);
	gtk_widget_set_valign(button_backward, GTK_ALIGN_START);
	gtk_widget_set_halign(button_forward, GTK_ALIGN_START);
	gtk_widget_set_valign(button_forward, GTK_ALIGN_START);
	gtk_widget_set_halign(colorbutton, GTK_ALIGN_START);
	gtk_widget_set_valign(colorbutton, GTK_ALIGN_START);
	gtk_widget_set_halign(savebutton, GTK_ALIGN_START);
	gtk_widget_set_valign(savebutton, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(box), savebutton);
	gtk_box_append(GTK_BOX(box), button_getcolor);
	gtk_box_append(GTK_BOX(box), button_backward);
	gtk_box_append(GTK_BOX(box), button_paint);
	gtk_box_append(GTK_BOX(box), button_forward);
	gtk_box_append(GTK_BOX(box), colorbutton);
	g_signal_connect(button_paint, "clicked", G_CALLBACK(paint_area_button), app);
	g_signal_connect(button_getcolor, "clicked", G_CALLBACK(get_color_button), app);
	gtk_widget_show(window);
	area = gtk_drawing_area_new();
	g_signal_connect(button_forward, "clicked", G_CALLBACK(forward_button), area);
	g_signal_connect(button_backward, "clicked", G_CALLBACK(backward_button), area);
	gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(area), 700);
	gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), 700);
  	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_function, NULL, NULL);
  	gtk_widget_set_size_request(GTK_WIDGET(area), 300, 400);
  	gtk_box_append(GTK_BOX(box), area);
  	GtkGesture *click = gtk_gesture_click_new();
    gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(click));
    g_signal_connect(click, "pressed", G_CALLBACK(clicked), area);
    g_signal_connect(click, "pressed", G_CALLBACK(getpixel), colorbutton);
    g_signal_connect(savebutton, "clicked", G_CALLBACK(savebutton_c), NULL);
}


int main(int argc, char *argv[]) {
	GtkApplication *app;
	int status;

	app = gtk_application_new("painter.org",
	                          G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}








