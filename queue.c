#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>



struct node_t *alloc_node(void)
{
    return malloc(sizeof(struct node_t));
}




void free_node(struct node_t *d)
{
    free(d);
}



void enqueue_node(struct node_t *new, struct queue_t *q)
{
    new->next = NULL;           // last node has not next

    if (q->tail)                // queue is not empty
        (q->tail)->next = new;
    else
        q->head = new;

    q->tail = new;
}




int dequeue_node(struct queue_t *q, struct node_t *d)
{
    if (q->head == NULL) {
        errno = EINVAL;
        return -1;              // queue is empty
    }

    if (q->head == q->tail)
        q->tail = NULL;         // there is only a node

    d = q->head;
    q->head = d->next;
    d->next = NULL;

    return 0;
}




int enqueue(void *pval, struct queue_t *q)
{
    struct node_t *new = alloc_node();

    if (new == NULL)
        return -1;              // allocation error

    new->value = pval;

    enqueue_node(new, q);

    return 0;
}



int dequeue(struct queue_t *q)
{
    struct node_t *d = NULL;

    if (dequeue_node(q, d) == -1) {
        errno = EINVAL;
        return -1;              // queue is empty
    }

    free_node(d);

    return 0;
}



void insert_after_node(struct node_t *pnew, struct node_t **pnext)
{
    pnew->next = *pnext;
    *pnext = pnew;
}


void insert_sorted_list(struct node_t *pnew, struct node_t **pp,
                        int (compare_funct) (void *, void *))
{
    struct node_t *p;
    for (p = *pp; p != NULL; pp = &p->next, p = p->next)
        if (compare_funct(pnew->value, p->value) < 0) {
            insert_after_node(pnew, pp);
            return;
        }
    insert_after_node(pnew, pp);
}


int prio_enqueue(void *pval, struct queue_t *q,
                 int (compare_funct) (void *, void *))
{
    struct node_t **phead = &q->head;
    struct node_t *pnew = alloc_node();

    if (pnew == NULL)
        return -1;

    pnew->value = pval;

    insert_sorted_list(pnew, phead, compare_funct);

    return 0;
}


void fprint_list(FILE * stream, struct node_t *h,
                 void (*print_funct) (FILE * stream, void *))
{
    struct node_t *p;
    for (p = h; p != NULL; p = p->next) {
        print_funct(stream, p->value);
        fprintf(stream, " ");
    }
    fprintf(stream, "\n");
}



void fprint_queue(FILE * stream, struct queue_t *queue,
                  void (*print_funct) (FILE * stream, void *))
{
    fprint_list(stream, queue->head, print_funct);
}
