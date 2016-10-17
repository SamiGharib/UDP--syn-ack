/* Implementation of a FIFO (as a double-chained linked list) queue to store packet */
#ifndef __FIFO_QUEUE_
#define __FIFO_QUEUE_

#include "packet_interface.h"
#include <sys/time.h>
#include <stdlib.h>

typedef struct queue{
		pkt_t *packet;
		struct timeval *tv;
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
queue_t *enqueue(queue_t *head,queue_t *tail,pkt_t *packet,struct timeval *tv);

/**
  * This method shall enqueue an node of the queue (so that it doesn't allocate
  * the memory needed to create the packet).
  * @param head: The head of the queue (i.e. the last element inserted in the queue). This element should not be NULL
  * @param tail: The tail of the queue (i.e. the older element inserted in the queue not yet dequeue). This element should not be NULL
  * @return After the execution, the head of the queue should be elem and elem->next should be head
  */
void re_enqueue(queue_t *head,queue_t *tail,queue_t *elem);

/**
  * @pre -
  * @post return the first element inserted in the queue and remove it from the queue
  */
queue_t *dequeue(queue_t *head,queue_t *tail);
#endif
