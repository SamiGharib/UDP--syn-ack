#include "queue.h"

queue_t *new_queue(){
		return calloc(1,sizeof(queue_t));
}

void queue_del(queue_t *queue){
		pkt_del(queue->packet);
		free(queue->tv);
		free(queue);
}

queue_t *enqueue(queue_t *head,queue_t *tail,pkt_t *packet,struct timeval *tv){
		queue_t *new_head = new_queue();
		if(new_head == NULL)
				return new_head;
		new_head->packet = packet;
		new_head->tv = tv;
		if(head == NULL && tail == NULL){
				head = new_head;
				tail = new_head;
		}
		else{
				new_head->next = head;
				head = new_head;
		}	
		return new_head;
}

queue_t *re_enqueue(queue_t *head,queue_t *tail,queue_t *elem){
		elem->next = head;
		head = elem;
		return head;
}
queue_t *dequeue(queue_t *head,queue_t *tail){
		if(tail == NULL)
				return NULL;
		if(tail == head)
				return tail;
		tail = tail->previous;
		queue_t *toReturn = tail->next;
		tail->next = NULL;
		return toReturn;
}
