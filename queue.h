#ifndef _QUEUE_H
#define _QUEUE_H

#include <stdio.h>

struct node_t {
    void *value;
    struct node_t *next;
};

struct queue_t {
	struct node_t *head;
	struct node_t *tail;
};


void enqueue_node(struct node_t *, struct queue_t *);
int dequeue_node(struct queue_t *, struct node_t *);
int enqueue(void *, struct queue_t *);
int dequeue(struct queue_t *);
int prio_enqueue(void *, struct queue_t *, int (void *, void *));
void fprint_queue(FILE *, struct queue_t *, void (*)(FILE *, void *));

#endif /* _QUEUE_H */
