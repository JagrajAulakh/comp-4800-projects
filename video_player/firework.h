#ifndef _FIREWORK
#define _FIREWORK

#include <gtk/gtk.h>

typedef struct firework Firework;

Firework *firework_new(int x, int y);
void firework_random_color(Firework *f);

int firework_get_x(Firework *f);
int firework_get_y(Firework *f);

int firework_get_r(Firework *f);
int firework_get_g(Firework *f);
int firework_get_b(Firework *f);
int firework_get_num_of_angles(Firework *f);
void firework_print(Firework *f);
double *firework_get_angles(Firework *f);
void firework_draw(cairo_t *cr, Firework *f);

double firework_get_percentage(Firework *f, int i);
void firework_make_thread(Firework *f);

#endif
