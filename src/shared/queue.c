#include "queue.h"

queue_t *new_queue(){
		return calloc(1,sizeof(queue_t));
}

void queue_del(queue_t *queue){
		pkt_del(queue->packet);
		free(queue->tv);
		free(queue);
}

queue_t *enqueue(queue_t **head,queue_t **tail,pkt_t *packet,struct timeval *tv){
		queue_t *new_head = new_queue();
		if(new_head == NULL)
				return new_head;
		new_head->packet = packet;
		new_head->tv = tv;
		if(*head == NULL && *tail == NULL){
				*head = new_head;
				*tail = new_head;
		}
		else{
				new_head->next = *head;
				*head = new_head;
		}	
		return new_head;
}

void re_enqueue(queue_t **head,queue_t **tail,queue_t *elem){
		elem->next = *head;
		*head = elem;
}
queue_t *dequeue(queue_t **head,queue_t **tail){
		if(*tail == NULL)
				return NULL;
		if(*tail == *head){
				queue_t *toReturn = *tail;
				*tail = NULL;
				*head = NULL;
				return toReturn;
		}
		*tail = (*tail)->previous;
		queue_t *toReturn = (*tail)->next;
		(*tail)->next = NULL;
		return toReturn;
}

void remove_queue(queue_t **head,queue_t **tail,queue_t *elem){
		if(elem == *head && elem == *tail){
				queue_del(elem);
				*head = NULL;
				*tail = NULL;
		}
		else if(elem == *head){
				*head = (*head)->next;
				(*head)->previous = NULL;
				queue_del(elem);
		}
		else if(elem == *tail){
				*tail = (*tail)->previous;
				(*tail)->next = NULL;
				queue_del(elem);
		}
		else{
				(elem->previous)->next = elem->next;
				(elem->next)->previous = elem->previous;
				queue_del(elem);
		}
}
