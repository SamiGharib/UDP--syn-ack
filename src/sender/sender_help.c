#include "sender_help.h"
#define DEBUG_HELP 1
uint8_t next_seqnum = 0; /* The next sequence number that will be used */
int free_to_go = 1; /* Number of packet we can send */
int actual_size_buffer = 0; /* Acutal size of the buffer in wich we store packet not yet acknowledge (i.e. the number of packet in the buffer) */
queue_t *head = NULL; /* Head of the queue used to store sent packet */
queue_t *tail = NULL; /* Tail of the queue used to store sent packet */
struct timeval time_out; /* Struct timeval representing the time a packet can be stored before being re-sent */
/* Buffer to store the last data sent. We use a specific buffer because when we receive a 0 window size, the queue might be empty */
uint8_t last_data[MAX_DATA_PACKET_SIZE];
uint16_t last_length;
sigjmp_buf env; /* Buffer to store environnement with sigseg */
int acked=0; /* last seqnum received from the receiver (so when we receive an ack, we can
			  acknowledge packet with seqnum such that : acked <= seqnum < seqnum_received */
/**
  * SIGALRM handler 
  */
static void sigalrm_handler(int signum){ siglongjmp(env,1);}
/**
  * This function shall return a pointer to a pkt_t struct containing the data as payload. 
  * 
  * @param data: Represent the payload of the packet
  * @return A new created pkt_t structure initialize with the right seqnum and length
  */
pkt_t *prepare_packet(const uint8_t *data,uint16_t length){
		pkt_t *pkt = pkt_new();
		if(pkt == NULL)
				return NULL;
		if(pkt_set_type(pkt,PTYPE_DATA) != PKT_OK)
				return NULL;
		if(pkt_set_window(pkt,0) != PKT_OK)
				return NULL;
		if(pkt_set_seqnum(pkt,next_seqnum) != PKT_OK)
				return NULL;
		next_seqnum = (next_seqnum+1) % 256;
		if(pkt_set_timestamp(pkt,0) != PKT_OK)
				return NULL;
		if(pkt_set_payload(pkt,data,length) != PKT_OK)
				return NULL;
		return pkt;
}
/**
  * This function shall create a queue_t structure, in order to store the packet juste sent, and enqueue it.
  *
  * @param pkt : The packet that need to be saved
  * @return If everything went right, 0 is return. Otherwise -1 is returned and information about the error is sent to stderr
  */
int add_to_queue(pkt_t *pkt){
		queue_t *elem = new_queue();
		elem->pkt = pkt;
		struct timeval *endtime = (struct timeval *)malloc(sizeof(struct timeval));
		int err = gettimeofday(endtime,NULL);
		if(err != 0){
				fprintf(stderr,"Error while setting the retransmission timer\n");
				return -1;
		}
		timeradd(endtime,&time_out,endtime);
		elem->end_time = endtime;
		enqueue(&head,&tail,elem);
		actual_size_buffer++;
		return 0;
}
/**
  * This function shall create a packet whit the data passed as argument, send it by the socket and store the packet in the queue.
  *
  * @param sfd : The file descriptor representing the socket
  * @param data : The buffer representing the data
  * @param length : The length of the buffer (i.e. the size in bytes of the data)
  * @return If the packet is send and store without problem, this method return 0. Otherwise, -1 is returned and an error message is print on stderr
  */
int send_packet(int sfd,uint8_t *data,uint16_t length){
		pkt_t *pkt = prepare_packet(data,length);
		if(pkt == NULL){
				fprintf(stderr,"Error while creating a packet\n");
				return -1;
		}
		size_t len = MAX_DATA_PACKET_SIZE;
		uint8_t data_toSend[MAX_DATA_PACKET_SIZE];
		memset((void *)data_toSend,0,MAX_DATA_PACKET_SIZE*sizeof(uint8_t));
		pkt_encode(pkt,data_toSend,&len);
		if(send(sfd,(void *)data_toSend,len*sizeof(uint8_t),0) == -1){
				fprintf(stderr,"Error while sending data to the socket : %s\n",strerror(errno));
				return -1;
		}
		/* Not the last packet */
		if(data != NULL){
			free_to_go --;
			/* Saving the last data and lenght of the data */
			memset((void *)last_data,0,MAX_DATA_PACKET_SIZE);
			memcpy((void *)last_data,(void *)data_toSend,len*sizeof(uint8_t));
			last_length = len;
			/* Adding the packet to the queue */
			return add_to_queue(pkt);
		}
		return 0;
}
/**
  * This function shall increase the timeout
  */
void increase_to(){
		struct timeval toAdd={0,time_out.tv_usec / INCREM_TIME_OUT};
		timeradd(&time_out,&toAdd,&time_out);
		if(time_out.tv_sec > 6 || (time_out.tv_sec == 6 && time_out.tv_usec > 0)){
				time_out.tv_sec = 6;
				time_out.tv_usec = 0;
		}
}

/**
  * This function shall decrease the time out
  */
void decrease_to(){
		struct timeval toSub={0,time_out.tv_usec / DECREM_TIME_OUT};
		timersub(&time_out,&toSub,&time_out);
		if(time_out.tv_sec < 2){
				time_out.tv_sec = 2;
				time_out.tv_usec = 0;
		}
}
/**
  * This function shall go through the queue and re-send the packet that has timed out. When a packet is no time out, 
  * we know that the packet behind (insert later) it has no timed out either.
  *
  * @param sfd: the socket to wich the data will be send
  * @return return 0 if no errors occurs, - 1 otherwise (and send information on stderr);
  */
int resend_data(int sfd){
		queue_t *itr = tail;
		while(itr != NULL){
				struct timeval current_time;
				int err = gettimeofday(&current_time,NULL);
				if(err != 0){
						fprintf(stderr,"Error while getting the current time to re-send data. Aborting operation\n");
						return -1;
				}
				/* Comparing time to the exepected time */
				struct timeval cmp;
				timersub(&current_time,itr->end_time,&cmp); /* We get the difference to update the time out struct */
				if(cmp.tv_sec > 0 || (cmp.tv_sec == 0 && cmp.tv_usec > 0)){
						size_t len = pkt_get_length(itr->pkt)+3*sizeof(uint32_t);
						uint8_t buf[len];
						memset((void *)buf,0,len*sizeof(uint8_t));
						if(pkt_encode(itr->pkt,buf,&len) != PKT_OK){
								fprintf(stderr,"Error while encoding packet for re-sending\n");
								return -1;
						}
						ssize_t b = send(sfd,(void *)buf,len,0);
						if(b == -1){
								fprintf(stderr,"Error while re-sending data over the socket : %s\n",strerror(errno));
								return -1;
						}
						itr = itr->previous;
						queue_t *new_head = dequeue(&head,&tail);
						if(new_head != NULL){
							add_to_queue(new_head->pkt);
							/* Incresing the time out */
							increase_to();
						}
				}
				else
						return 0;
		}
		return 0;
}

/**
  * This function shall go through the queue and remove eleme that we can acknowledge. A packet can be acknowledge if and only if is sequence number is less than the sequence number receive in the last 
  * acknowledgment packet.
  *
  * @param last_seqnum : the last sequence number receive in an acknowledgment packet
  * @return -
  */
void acknowledge_pkt(int last_seqnum){
		if(last_seqnum == 0)
				last_seqnum = 256;
		queue_t *itr = tail;
		while(itr != NULL){
				if(pkt_get_seqnum(itr->pkt) < last_seqnum && pkt_get_seqnum(itr->pkt) >= acked){
						printf("acknowledge : %d\n",pkt_get_seqnum(itr->pkt));
						queue_t *toRemove = itr;
						itr = itr->previous;
						remove_elem(&head,&tail,toRemove);
						decrease_to(); /* Decreasing the time out */
						free_to_go++;
						actual_size_buffer--;
				}
				else
						itr = itr->previous;
		}
		if(last_seqnum == 255)
				acked = 0;
		else
				acked = last_seqnum-1;
}
/** 
  * This function shall read data from stdin and send it by the socket to the defined address.
  * Once EOF is encountered, the function shall send an end-of-communication segment if and only if all the data has been acknowledged.
  *
  * @param dest_addr : a pointer to a zone representing the hostname to wich we want to send data. It can be a name domain like "www.mysupersite.com", an IPV6 address, etc.
  * @param port : the port by wich we will send the data read from stdin.
  * @return If all the data has been sent without error, then 0 is returned. If an error has occured while the process, -1 is return and the connection is ended by an "abrupt" disconnection (i.e. data may be lost (never re-sent, never read from stdin, etc.) All error messages are sent to stderr
  */
int send_data(const char *dest_addr,int port){
		/* Resolving the address */
		struct sockaddr_in6 addr;
		const char *err = real_address(dest_addr,&addr);
		if(err != NULL){
				fprintf(stderr,"Error while resolving the hostname : %s\n",strerror(errno));
				return -1;
		}
		/* Creating the socket */
		int sfd = create_socket(NULL,-1,&addr,port);
		if(sfd < 0){
				fprintf(stderr,"Error while resolving the hostname : %s\n",strerror(errno));
				return -1;
		}
		/* Setting up the various buffer to read data */
		uint8_t data_buffer[MAX_PAYLOAD_SIZE];
		memset((void *)data_buffer,0,MAX_PAYLOAD_SIZE*sizeof(uint8_t));
		uint8_t ack_buffer[INFO_PACKET_SIZE];
		memset((void *)ack_buffer,0,INFO_PACKET_SIZE*sizeof(uint8_t));
		/* Setting the fd_set and files descriptors for the select() call */
		fd_set readfds;
		FD_ZERO(&readfds);
		/* To store the return value of read/write */
		ssize_t nBytes;
		/* Starting multiplexing the files descriptors */
		struct timeval tv={0,0};
		/* Setting up the sig handler */
		if(signal(SIGALRM,sigalrm_handler) == SIG_ERR){
				fprintf(stderr,"Error while setting up the handler for SIGALRM : %s\n",strerror(errno));
				return -1;
		}
		int end_of_data=0; /* flag to detect end of data */
		int try_end_communication = 0; /* Number of attemps to terminate properly the connection */
		while(1){
				if(end_of_data && head == NULL && tail == NULL){
						if(try_end_communication == 2){
								fprintf(stderr,"No acknowledgment receive after 3 end-of-communication packet. Disconnecting...\n");
								return 0;
						}
						if(send_packet(sfd,NULL,0) == -1){
								fprintf(stderr,"Error while sending the end-of-communication packet. Disconnecting brutally\n");
								return 0;
						}
						tv.tv_sec = 2;
						tv.tv_usec = 5;
						try_end_communication++;
				}
				FD_SET(fileno(stdin),&readfds);
				FD_SET(sfd,&readfds);
				int rv = select(sfd+1,&readfds,NULL,NULL,&tv);
				if(rv == -1){
						fprintf(stderr,"Error while multiplexing the socket and stdin : %s\n",strerror(errno));
						return -1;
				}
				/* Data ready to be read on stdin and send to the dest */
				if(FD_ISSET(fileno(stdin),&readfds) && free_to_go != 0 && actual_size_buffer <= MAX_WINDOW_SIZE){
						nBytes = read(fileno(stdin),(char *)data_buffer,MAX_PAYLOAD_SIZE);
						if(nBytes == -1){
								fprintf(stderr,"Error while reading the data from stdin: %s. Terminating the connection\n",strerror(errno));
								return -1;
						}
						else if(nBytes == 0){
								end_of_data = 1;
						}
						else{
								if(send_packet(sfd,data_buffer,nBytes) == -1)
										return -1;
						}
				}
				if(FD_ISSET(sfd,&readfds)){
						nBytes = recv(sfd,(char *)ack_buffer,INFO_PACKET_SIZE,0);
						if(nBytes == -1){
								fprintf(stderr,"Errror while reading data from the socket : %s\n",strerror(errno));
								return -1;
						}
						pkt_t *pkt = pkt_new();
						if(pkt_decode(ack_buffer,nBytes,pkt) != PKT_OK){
								fprintf(stderr,"Error while decoding an ack. Discarded\n");
								continue;
						}
						else{
								printf("Receiving ack for packet : %d\n",pkt_get_seqnum(pkt));
								if(end_of_data && head == NULL && tail == NULL)
										return 0;
								acknowledge_pkt(pkt_get_seqnum(pkt));
								free_to_go = pkt_get_window(pkt);
								if(pkt_get_window(pkt) == 0){
										if(sigsetjmp(env,1)==0)
												alarm(4);
										else{
												if(send(sfd,(void *)last_data,last_length,0) == -1)
														fprintf(stderr,"Error while waking up the receiver (window == 0) after 4 second : %s\n",strerror(errno));
												alarm(4);
										}
								}
								else{
									alarm(0);
								}
								
						}
				}
				if(free_to_go != 0 && actual_size_buffer > 0)
						resend_data(sfd);
		}
}
