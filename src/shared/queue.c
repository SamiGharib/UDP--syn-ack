#include <stdio.h>
#include "queue.h"

queue_t *new_queue(){
		return calloc(1,sizeof(queue_t));
}

void queue_del(queue_t *queue){
		pkt_del(queue->pkt);
		free(queue->end_time);
		free(queue);
}

void enqueue(queue_t **head,queue_t **tail,queue_t *elem){
		if(*head == NULL && *tail == NULL){
				*head = elem;
				*tail = elem;
				(*head)->previous = NULL;
				(*head)->next = NULL;
				(*tail)->next = NULL;
				(*tail)->previous = NULL;
		}
		else{
				elem->previous = NULL;
				elem->next = *head;
				(*head)->previous = elem;
				*head = elem;
		}
}

queue_t *dequeue(queue_t **head,queue_t **tail){
		if(*tail == NULL)
				return NULL;
		else if(*head == *tail){
				queue_t *toReturn = *tail;
				*tail = NULL;
				*head = NULL;
				return toReturn;
		}
		else{
				queue_t *toReturn = *tail;
				*tail = (*tail)->previous;
				(*tail)->next = NULL;
				return toReturn;
		}
}

void remove_elem(queue_t **head,queue_t **tail,queue_t *elem){
		if(*head == NULL && *tail == NULL)
				return;
		if(elem == *tail){
				elem = dequeue(head,tail);
				queue_del(elem);
		}
		else if(elem == *head){
				*head = elem->next;
				elem->next = NULL;
				(*head)->previous = NULL;
				queue_del(elem);
		}
		else{
				(elem->previous)->next = elem->next;
				(elem->next)->previous = elem->previous;
				elem->next = NULL;
				elem->previous = NULL;
				queue_del(elem);
		}
}
