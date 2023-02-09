#ifndef _QUEUE
#define _QUEUE

#include "firework.h"

typedef struct queue Queue;
typedef struct node Node;

Queue *queue_new();
void queue_add(Queue *q, Firework *f);
Firework *queue_pop(Queue *q);
void queue_print(Queue *q);
int queue_size(Queue *q);
Node *queue_get_head(Queue *q);
Node *node_get_next(Node *n);
Firework *node_get_firework(Node *n);
void queue_delete(Queue *q, Firework *f);
int queue_get_lock(Queue *q);

#endif
