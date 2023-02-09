#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "firework.h"

struct node {
	struct node *next;
	Firework *firework;
};

struct queue {
	struct node *head;
};

Queue *queue_new() {
	Queue *q = (Queue *)malloc(sizeof(Queue));
	q->head = NULL;
	return q;
}

void queue_add(Queue *q, Firework *f) {
	if (q->head == NULL) {
		q->head = (Node *)malloc(sizeof(Node));
		q->head->firework = f;
		q->head->next = NULL;
		return;
	}

	Node *tmp = q->head;
	while (tmp->next != NULL) {
		tmp = tmp->next;
	}

	tmp->next = (Node *)malloc(sizeof(Node));
	tmp->next->firework = f;
	tmp->next->next = NULL;
}

Firework *queue_pop(Queue *q) {
	if (q->head == NULL) {
		return NULL;
	}
	Node *tmp = q->head;
	q->head = q->head->next;

	Firework *f = tmp->firework;
	free(tmp);
	return f;
}

void queue_print(Queue *q) {
	if (!q->head) {
		puts("{}");
		return;
	}

	int i = 0;
	Node *tmp = q->head;
	printf("{");
	while (1) {
		printf("(%d %d), ", firework_get_x(tmp->firework),
		       firework_get_y(tmp->firework));
		if (!tmp->next) break;
		tmp = tmp->next;
		i++;
	}
	printf("\b\b}\n");
}

Node *queue_get_head(Queue *q) { return q->head; }

Node *node_get_next(Node *n) {
	if (!n || !n->next) return NULL;
	return n->next;
}

Firework *node_get_firework(Node *n) { return n->firework; }

// int main() {
// 	Queue *q = queue_new();
//
// 	queue_print(q);
//
// 	Firework *f1 = firework_new(1, 0);
// 	queue_add(q, f1);
// 	queue_print(q);
//
// 	Firework *f2 = firework_new(3, 2);
// 	queue_add(q, f2);
// 	queue_print(q);
//
// 	Firework *p = queue_pop(q);
// 	printf("Popped (%d %d)\n", firework_get_x(p),
// 	       firework_get_y(p));
// 	queue_print(q);
// }
