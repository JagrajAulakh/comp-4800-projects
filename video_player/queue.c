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
	int lock;
};

Queue *queue_new() {
	Queue *q = (Queue *)malloc(sizeof(Queue));
	q->head = NULL;
	q->lock = 0;
	return q;
}

void queue_add(Queue *q, Firework *f) {
	while (q->lock) {
	}
	q->lock = 1;
	if (q->head == NULL) {
		q->head = (Node *)malloc(sizeof(Node));
		q->head->firework = f;
		q->head->next = NULL;
		q->lock = 0;
		return;
	}

	Node *tmp = q->head;
	while (tmp->next != NULL) {
		tmp = tmp->next;
	}

	tmp->next = (Node *)malloc(sizeof(Node));
	tmp->next->firework = f;
	tmp->next->next = NULL;
	q->lock = 0;
}

Firework *queue_pop(Queue *q) {
	while (q->lock) {
	}
	q->lock = 1;
	if (q->head == NULL) {
		return NULL;
	}
	Node *tmp = q->head;
	q->head = q->head->next;

	Firework *f = tmp->firework;
	free(tmp);
	q->lock = 0;
	return f;
}

void queue_delete(Queue *q, Firework *f) {
	while (q->lock) {
	}

	if (!q->head) return;
	q->lock = 1;
	if (q->head->firework == f) {
		Node *toFree = q->head;
		q->head = toFree->next;
		free(toFree);
		q->lock = 0;
		return;
	}

	Node *tmp = q->head, *toFree;
	while (1) {
		if (tmp->next && f == tmp->next->firework) {
			toFree = tmp->next;
			tmp->next = tmp->next->next;
			free(toFree);
			break;
		}

		tmp = tmp->next;
		if (!tmp) {
			break;
		}
	}

	q->lock = 0;
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

int queue_size(Queue *q) {
	if (!q->head) return 0;

	int size = 0;
	Node *tmp = q->head;
	while (1) {
		if (!tmp) break;
		size++;
		tmp = tmp->next;
	}

	return size;
}

Node *queue_get_head(Queue *q) { return q->head; }

Node *node_get_next(Node *n) {
	if (!n || !n->next) return NULL;
	return n->next;
}

Firework *node_get_firework(Node *n) { return n->firework; }

int queue_get_lock(Queue *q) { return q->lock; }

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
