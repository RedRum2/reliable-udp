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


/* Function:	insert_after_node
 * --------------------------------------------------------------
 * Insert the new node after a node.
 *
 * Parameters:
 * 		pnew	address of the new node
 * 		pnext	address of the previous node's next field
 */
void insert_after_node(struct node_t *pnew, struct node_t **pnext)
{
    pnew->next = *pnext;
    *pnext = pnew;
}


/* Function:	delete_after_node	
 * --------------------------------------------------------------
 * Delete the next node of a node.
 *
 * Parameters:
 * 		ppos	address of the node's next field
 *
 * Returns:
 * 		the deleted node
 */
struct node_t *remove_after_node(struct node_t **ppos)
{
    struct node_t *d = *ppos;
    *ppos = d->next;
	return d;
}


/*
 * Function:	insert_sorted_list
 * ----------------------------------------------------------------
 */
void insert_sorted_list(struct node_t *pnew, struct node_t **pnext,
                        int (compare_funct) (void *, void *))
{
    struct node_t *p;

    for (p = *pnext; p != NULL; pnext = &p->next, p = p->next)
        if (compare_funct(pnew->value, p->value) < 0) {
			// insert before p (pnext is the pointer to the
			// next field of the p's previous node)
            insert_after_node(pnew, pnext);
            return;
        }

	// insert at the bottom
    insert_after_node(pnew, pnext);
}


/*
 * Function:	find_ppos
 * ----------------------------------------------------------------
 * Find the previos node's next field of the node with the same value
 * as pnode.
 *
 * Parameters:
 * 		pnode			the node to compare
 * 		pnext			a pointer to address of the first node to check
 * 		compare_funct	function that compares the nodes' values
 *
 * Returns:
 * 		the address of the previous node's next field;
 *		NULL if there is not a matching node.
 */
struct node_t **find_ppos(struct node_t *pnode, struct node_t **pnext,
                           int (compare_funct) (void *, void *))
{
    struct node_t *p;

    for (p = *pnext; p != NULL; pnext = &p->next, p = p->next) {
        if (compare_funct(pnode->value, p->value) == 0) 
            return pnext;
	}
	return NULL;
}


/*
 * Function:	remove_node
 * -------------------------------------------------------------
 * Remove the node mathing the specified value.
 * The match is checked by the compare function.
 *
 * Parameters:
 * 		pval			value of the node to remove
 * 		q				address of the queue
 * 		compare_funct	function that compares the values
 *
 * 	Returns:
 * 		0	on success
 * 		-1	if there is not a node with the value	
 */
int remove_node(void *pval, struct queue_t *q, int (compare_funct) (void *, void *))
{
    struct node_t **phead = &q->head;
    struct node_t *pnew = alloc_node();

    if (pnew == NULL)
        return -1;

    pnew->value = pval;

	struct node_t **ppos = find_ppos(pnew, phead, compare_funct);
	
	if (ppos == NULL)
		return -1;

	free(remove_after_node(ppos));	

    return 0;
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
	fputs("Q: ", stream);
    fprint_list(stream, queue->head, print_funct);
}
