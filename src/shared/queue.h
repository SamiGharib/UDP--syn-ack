/* Implementation of a FIFO (as a double-chained linked list) queue to store packet */
#ifndef __FIFO_QUEUE_
#define __FIFO_QUEUE_

#include "packet_interface.h"
#include <sys/time.h>
#include <stdlib.h>

typedef struct queue{
		pkt_t *pkt;
		struct timeval *end_time;
		struct queue *next;
		struct queue *previous;
} queue_t;

/**
  * @pre -
  * @post return a pointer to a queue_t struct or NULL if an 
  *			error occured.
  */
queue_t *new_queue();

/**
  * @pre queue != NULL
  * @post Free all ressources used by a node in the queue (including the 
  *		  ressources needed by the packet)
  */
void queue_del(queue_t *queue);

/**
  * @pre packet != NULL && tv != NULL
  * @post If the initial queue was empty, the queue now contain one element (pointed by head).
  *		  This element has the given arguments as member and head->next == NULL.
  *		  If the list is not empty, then <head> point to a new element (wich member are the
  *		  given parameter) and head->next = <old_head>
  */
void enqueue(queue_t **head,queue_t **tail,queue_t *elem);


/**
  * @pre -
  * @post return the first element inserted in the queue and remove it from the queue
  */
queue_t *dequeue(queue_t **head,queue_t **tail);

/**
  * This function shall remove an arbitrary given element from the queue (even the head or the tail)
  * @param head: This is a pointer to the head of the queue. It should not be NULL
  * @param tail: This is a pointer to the tail of the queue. It should not be NULL
  * @param elem: This is a pointer to the element to remove from the queue. It should not be NULL
  * @return -
  */
void remove_elem(queue_t **head,queue_t **tail,queue_t *elem);
#endif
