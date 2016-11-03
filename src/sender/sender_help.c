#define _DEFAULT_SOURCE
#include "sender_help.h"
#define DEBUG_HELP 1
#define MASK_LESS_BITS(k,n) (k & ((1 << n) -1)) /*Mask to get the n last bit of k */
uint8_t next_seqnum = 0; /* The next sequence number that will be used */
int window_receiver = 1; /* window size of the receiver */
int actual_size_buffer = 0; /* Acutal size of the buffer in wich we store packet not yet acknowledge (i.e. the number of packet in the buffer) */
int pkt_count = 1; /* Count the number of packet sent after receiving an ack. This value cannot be larger than window_receiver */
struct timeval time_out; /* Struct timeval representing the time a packet can be stored before being re-sent */
/* Buffer to store the last data sent. We use a specific buffer because when we receive a 0 window size, the queue might be empty */
uint8_t last_data[MAX_DATA_PACKET_SIZE];
uint16_t last_length;
sigjmp_buf env; /* Buffer to store environnement with sigseg */
int next_expected=0; /* last seqnum received from the receiver (so when we receive an ack, we can
			  acknowledge packet with seqnum such that : next_expected <= seqnum < seqnum_received */
/* Variable for the queue */
struct entry{
	TAILQ_ENTRY(entry) entries;
	pkt_t *pkt;
};

TAILQ_HEAD(stailhead,entry);
/**
  * SIGALRM handler 
  */
static void sigalrm_handler(){ siglongjmp(env,1);}
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
	if(data != NULL)
		next_seqnum = (next_seqnum+1) % 256;
	if(pkt_set_timestamp(pkt,0) != PKT_OK)
		return NULL;
	if(pkt_set_payload(pkt,data,length) != PKT_OK)
		return NULL;
	return pkt;
}

/**
 * This function shall create a packet whit the data passed as argument, send it by the socket and store the packet in the queue.
 *
 * @param sfd : The file descriptor representing the socket
 * @param data : The buffer representing the data
 * @param length : The length of the buffer (i.e. the size in bytes of the data)
 * @return If the packet is send and store without problem, this method return 0. Otherwise, -1 is returned and an error message is print on stderr
 */
static int send_packet(int sfd,uint8_t *data,uint16_t length,struct stailhead *head){
	pkt_t *pkt = prepare_packet(data,length);
	if(pkt == NULL){
		fprintf(stderr,"Error while creating a packet\n");
		return -1;
	}
	/* Initializing the end_time to put the timestamp */
	struct timeval ts;
	int err = gettimeofday(&ts,NULL);
	if(err != 0){
		fprintf(stderr,"Error while getting time of day to send packet\n");
		return -1;
	}
	pkt->timestamp = (((uint32_t)MASK_LESS_BITS(ts.tv_sec,12)) << 20) | (uint32_t)MASK_LESS_BITS(ts.tv_usec,20);
	size_t len = MAX_DATA_PACKET_SIZE;
	uint8_t data_toSend[MAX_DATA_PACKET_SIZE];
	memset((void *)data_toSend,0,MAX_DATA_PACKET_SIZE*sizeof(uint8_t));
	pkt_encode(pkt,data_toSend,&len);
	if(send(sfd,(void *)data_toSend,len*sizeof(uint8_t),0) == -1){
		fprintf(stderr,"Error while sending data to the socket : %s\n",strerror(errno));
		return -1;
	}
	pkt_count++;
	/* Not the last packet */
	if(data != NULL){
		/* Saving the last data and lenght of the data */
		memset((void *)last_data,0,MAX_DATA_PACKET_SIZE);
		memcpy((void *)last_data,(void *)data_toSend,len*sizeof(uint8_t));
		last_length = len;
		/* Adding the packet to the queue */
		struct entry *new_entry = (struct entry *)malloc(sizeof(struct entry));
		new_entry->pkt = pkt;
		TAILQ_INSERT_TAIL(head,new_entry,entries);
		actual_size_buffer++;
		return 0;
	}
	else
		pkt_del(pkt);
	return 0;
}
/**
  * This function shall increase the timeout
  */
static void increase_to(){
	long timeInMs = (time_out.tv_sec*1000000 + time_out.tv_usec)/INCREM_TIME_OUT;
	struct timeval tmp = {timeInMs/1000000, timeInMs - timeInMs/1000000};
	timeradd(&time_out,&tmp,&time_out);
	if(time_out.tv_sec > MAX_TIME_SEC || (time_out.tv_sec == MAX_TIME_SEC && time_out.tv_usec > MAX_TIME_USEC)){
		time_out.tv_sec = MAX_TIME_SEC;
		time_out.tv_usec = MAX_TIME_USEC;
	}
}

/**
  * This function shall decrease the time out
  */
static void decrease_to(pkt_t *pkt){
	struct timeval pkt_time = {MASK_LESS_BITS(pkt->timestamp >> 20,12)+MASK_LESS_BITS(time_out.tv_sec,12),MASK_LESS_BITS(pkt->timestamp,20)+MASK_LESS_BITS(time_out.tv_usec,20)},current_time,tmp;
	if(pkt_time.tv_usec >= 1000000){
		pkt_time.tv_sec += pkt_time.tv_usec / 1000000;
		pkt_time.tv_usec -= (pkt_time.tv_usec / 1000000) * 1000000;
	}
	int err = gettimeofday(&current_time,NULL);
	if(err != 0){
		fprintf(stderr,"Error while getting time of day before decreasing time out. Aborting\n");
		return;
	}
	if((MASK_LESS_BITS(pkt_time.tv_sec,13) & (1 << 13)) != 0)
		tmp.tv_sec = MASK_LESS_BITS(pkt_time.tv_sec,13) - MASK_LESS_BITS(current_time.tv_sec,13);
	else
		tmp.tv_sec = MASK_LESS_BITS(pkt_time.tv_sec,12)- MASK_LESS_BITS(current_time.tv_sec,12);
	tmp.tv_usec = MASK_LESS_BITS(pkt_time.tv_sec,20) - MASK_LESS_BITS(current_time.tv_sec,20);
	if(tmp.tv_usec < 0){
		tmp.tv_sec --;
		tmp.tv_usec += 999999;
	}
	long timeInMs = (tmp.tv_sec*1000000 +tmp.tv_usec)/DECREM_TIME_OUT;
	struct timeval decrem = {timeInMs/1000000,timeInMs - timeInMs/1000000};
	timersub(&time_out,&decrem,&time_out);
	if(time_out.tv_sec < TIME_SEC || (time_out.tv_sec == TIME_SEC && time_out.tv_sec < TIME_USEC)){
		time_out.tv_sec = TIME_SEC;
		time_out.tv_usec = TIME_USEC;
	}

}
/**
  * This function shall see if a packet has timed out (i.e. he has been sent since more that time_out.tv_sec sec and time_out.tv_usec micro-sec)
  * 
  * @param pkt: A pointer to the packet that'll be check
  *
  * @return If the packet has timeout then the function return 1. 0 is returned otherwise and if an error occured, -1 is returned and informations are printed on stderr
  */
static int has_time_out(pkt_t *pkt){
	struct timeval current_time;
	int err = gettimeofday(&current_time,NULL);
	if(err != 0){
		fprintf(stderr,"Error while getting time of day to check if the ack w/ seqnum %d is an old ack.\n",pkt_get_seqnum(pkt));
		return -1;
	}
	uint32_t ts = (uint32_t)MASK_LESS_BITS(pkt->timestamp >> 20,12), tus = (uint32_t)MASK_LESS_BITS(pkt->timestamp,20);
	uint32_t cs = (uint32_t)MASK_LESS_BITS(current_time.tv_sec,12), cus = (uint32_t)MASK_LESS_BITS(current_time.tv_usec,20);
	uint32_t sec = cs - ts;
	uint32_t usec = cus -tus;
	if( sec > time_out.tv_sec || (sec == time_out.tv_sec && usec >= time_out.tv_usec))
		return 1;
	return 0;
}

/**
  * This function shall go through the queue and re-send the packet that has timed out. When a packet is no time out, 
  * we know that the packet behind (insert later) it has no timed out either.
  *
  * @param sfd: the socket to wich the data will be send
  * @return return 0 if no errors occurs, - 1 otherwise (and send information on stderr);
  */
static int resend_data(int sfd,struct stailhead *head){
	int first=1;
	struct entry *itr = head->tqh_first;
	int i=0;
	for(i=0;i<actual_size_buffer && itr != NULL;i++){
		if(has_time_out(itr->pkt)){
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
			struct timeval ct;
			int err = gettimeofday(&ct,NULL);
			if(err != 0){
				fprintf(stderr,"Error while getting time of day to update timestamp of a packet after re-sending\n");
				return -1;
			}
			pkt_set_timestamp(itr->pkt,((uint32_t)(MASK_LESS_BITS(ct.tv_sec,12)) << 20) |(uint32_t)MASK_LESS_BITS(ct.tv_usec,20));
			TAILQ_REMOVE(head,itr,entries);
			TAILQ_INSERT_TAIL(head,itr,entries);
			if(first){
				increase_to();
				first = 0;
			}
		}
		itr = itr->entries.tqe_next;
	}
	return 0;
}

static void remove_from_queue(int lo,int hi,struct stailhead *head){
	struct entry *itr = head->tqh_first;
	while(itr != NULL){
		struct entry *next = itr->entries.tqe_next;
		if(pkt_get_seqnum(itr->pkt) < hi && pkt_get_seqnum(itr->pkt) >= lo){
			TAILQ_REMOVE(head,itr,entries);
			decrease_to(itr->pkt); /* Decreasing the time out */
			pkt_del(itr->pkt);
			free(itr);
			actual_size_buffer--;
		}
		itr = next;
	}
}
				
/**
  * This function shall go through the queue and remove eleme that we can acknowledge. A packet can be acknowledge if and only if is sequence number is less than the sequence number receive in the last 
  * acknowledgment packet.
  *
  * @param last_seqnum : the last sequence number receive in an acknowledgment packet
  * @return -
  */
static void acknowledge_pkt(uint8_t last_seqnum,struct stailhead *head){
	if(last_seqnum == 0 && next_expected != 0){
		remove_from_queue(next_expected,256,head);
	}
	else if(last_seqnum < next_expected){
		remove_from_queue(next_expected,256,head);
		remove_from_queue(0,last_seqnum,head);
	}
	else{
		remove_from_queue(next_expected,last_seqnum,head);
	}
	next_expected = last_seqnum;
}

/**
  * This function shall check the timestamp of the acknowledgment to see if this is an old ACK
  *
  * @param pkt: a pointer to the acknowledgment
  * @return: if the packet is an old ack, the function return 1. If the acknowledgment is not an old ack, 0 is returned. If an error occured, -1 is returned.
  */
static int is_old_ack(pkt_t *pkt){
	if(next_expected+32 <= 255){
		if(pkt_get_seqnum(pkt) <= next_expected || pkt_get_seqnum(pkt) > next_expected+32)
			return 1;
	}
	else{
		if(pkt_get_seqnum(pkt) <= next_expected && pkt_get_seqnum(pkt) > 32-(255-next_expected))
			return 1;
	}
	struct timeval current_time;
	int err = gettimeofday(&current_time,NULL);
	if(err != 0)
		return -1;
	uint32_t ts = (uint32_t)pkt->timestamp >> 20, tus = (uint32_t)MASK_LESS_BITS(pkt->timestamp,20);
	uint32_t cs = (uint32_t)MASK_LESS_BITS(current_time.tv_sec,12), cus =(uint32_t) MASK_LESS_BITS(current_time.tv_usec,20);
	uint32_t sec = cs - ts;
	uint32_t usec = cus -tus;
	if( sec > MSL_SEC || (sec == MSL_SEC && usec >= MSL_USEC)){
		fprintf(stderr,"Ack has spent to much time in the network (%u %u). Discarded\n",sec,usec);
		return 1;
	}
	return 0;
}

/**
  * This function shall set the timer for the select call. If the first packet has timed out, the timer is set to 0 s and 0us
  *
  * @param tv: a pointer to the timer
  * @param head: a pointer to the head of the queue (to get the timestamp of the first packet)
  *
  * @return -1 if an error occur, 0 otherwise
  */
static int set_timer(struct timeval *tv,struct stailhead *head){
	/* Initiating the timer for the select() call to the time the first packet in the queue has left before TO */
	struct timeval t;
	int err = gettimeofday(&t,NULL);
	if(err != 0){
		fprintf(stderr,"Error while getting time of day to set timer for select()\n");
		return -1;
	}
	uint32_t ds = (uint32_t)MASK_LESS_BITS(t.tv_sec,12) - (uint32_t)MASK_LESS_BITS((head->tqh_first)->pkt->timestamp >> 20,12);
	uint32_t dus = (uint32_t)MASK_LESS_BITS(t.tv_usec,20) - (uint32_t)MASK_LESS_BITS((head->tqh_first)->pkt->timestamp,20);
	if(dus < 0){
		ds--;
		dus += 999999;
	}
	if(ds > MASK_LESS_BITS(time_out.tv_sec,12) || (ds == MASK_LESS_BITS(time_out.tv_sec,12) && dus > MASK_LESS_BITS(time_out.tv_usec,20))){
		tv->tv_sec = 0;
		tv->tv_usec = 0;
	}
	else{
		tv->tv_sec = ds;
		tv->tv_usec = dus;
	}
	return 0;
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
		fprintf(stderr,"Error while resolving the hostname : %s\n",err);
		return -1;
	}
	/* Creating the socket */
	int sfd = create_socket(NULL,-1,&addr,port);
	if(sfd < 0){
		fprintf(stderr,"Error while resolving the hostname : %s\n",strerror(errno));
		close(sfd);
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
	struct timeval tv;
	/* Setting up the sig handler */
	if(signal(SIGALRM,sigalrm_handler) == SIG_ERR){
		fprintf(stderr,"Error while setting up the handler for SIGALRM : %s\n",strerror(errno));
		close(sfd);
		return -1;
	}
	/* Initializing the queue */
	struct stailhead head;
	TAILQ_INIT(&head);
	/* Flags */
	int end_of_data=0; /* flag to detect end of data */
	int try_end_communication = 0; /* Number of attemps to terminate properly the connection */
	/* Initialising time out */
	time_out.tv_sec = TIME_SEC;
	time_out.tv_usec = TIME_USEC;
	while(1){
		if(end_of_data && actual_size_buffer == 0){ 
			fprintf(stderr,"Entering end-of-communication routines for the %d times\n",try_end_communication+1);
			fflush(stderr);
			if(try_end_communication == 2){
				fprintf(stderr,"No acknowledgment receive after 3 end-of-communication packet. Disconnecting...\n");
				int down = close(sfd);
				if(down == -1){
					fprintf(stderr,"Error while closing socket : %s\n",strerror(errno));
					return -1;
				}
				return 0;
			}
			if(send_packet(sfd,NULL,0,&head) == -1){
				fprintf(stderr,"Error while sending the end-of-communication packet. Disconnecting brutally\n");
				return 0;
			}
			try_end_communication++;
			tv.tv_sec = MAX_TIME_SEC;
			tv.tv_usec = MAX_TIME_USEC;
		}
		if(head.tqh_first != NULL){
			if(set_timer(&tv,&head) == -1)
				continue;
		}
		else{
			tv.tv_sec = 0;
			tv.tv_usec = 0;
		}
		FD_SET(fileno(stdin),&readfds);
		FD_SET(sfd,&readfds);
		if(DEBUG_HELP){
			fprintf(stderr,"timer : %ld %ld\n",tv.tv_sec,tv.tv_usec);
			fflush(stderr);
		}
		int rv = select(sfd+1,&readfds,NULL,NULL,&tv);
		if(rv == -1){
			fprintf(stderr,"Error while multiplexing the socket and stdin : %s\n",strerror(errno));
			close(sfd);
			return -1;
		}
		if(rv == 0 && head.tqh_first != NULL){
			resend_data(sfd,&head);
			FD_CLR(fileno(stdin),&readfds);
			FD_CLR(sfd,&readfds);
			continue;
		}
		/* Receiving ack */
		if(FD_ISSET(sfd,&readfds)){
			if(DEBUG_HELP){
				fprintf(stderr,"ack received\n");
				fflush(stderr);
			}
			nBytes = recv(sfd,(char *)ack_buffer,INFO_PACKET_SIZE,0);
			if(nBytes == -1){
				fprintf(stderr,"Errror while reading data from the socket : %s\n",strerror(errno));
				close(sfd);
				return -1;
			}
			pkt_t *pkt = pkt_new();
			if(pkt_decode(ack_buffer,nBytes,pkt) != PKT_OK){
				fprintf(stderr,"Error while decoding an ack. Discarded\n");
				pkt_del(pkt);
				FD_CLR(fileno(stdin),&readfds);
				FD_CLR(sfd,&readfds);
				continue;
			}
			else if(is_old_ack(pkt)){
				fprintf(stderr,"Receiving old ack. Discarded\n");
				pkt_del(pkt);
				FD_CLR(fileno(stdin),&readfds);
				FD_CLR(sfd,&readfds);
				continue;
			}
			else{
				if(end_of_data && actual_size_buffer == 0){
					pkt_del(pkt);
					int down = close(sfd);
					fprintf(stderr,"receiving ack for end-of-communication packet. Closing connection\n");
					fflush(stderr);
					if(down == -1){
						fprintf(stderr,"Error while closing socket : %s\n",strerror(errno));
						return -1;
					}
					return 0;
				}
				window_receiver = pkt_get_window(pkt);
				pkt_count = 0;
				acknowledge_pkt(pkt_get_seqnum(pkt),&head);
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
			pkt_del(pkt);
		}
		/* Data ready to be read on stdin and send to the dest */
		if(FD_ISSET(fileno(stdin),&readfds) && window_receiver != 0 && pkt_count <= window_receiver && actual_size_buffer <= MAX_WINDOW_SIZE){
			if(DEBUG_HELP){
				fprintf(stderr,"data ready to be sent\n");
				fflush(stderr);
			}
			nBytes = read(fileno(stdin),(char *)data_buffer,MAX_PAYLOAD_SIZE);
			if(nBytes == -1){
				fprintf(stderr,"Error while reading the data from stdin: %s. Terminating the connection\n",strerror(errno));
				close(sfd);
				return -1;
			}
			else if(nBytes == 0){
				end_of_data = 1;
			}
			else{
				if(send_packet(sfd,data_buffer,nBytes,&head) == -1){
					close(sfd);
					return -1;
				}
				pkt_count ++;
			}
		}
		resend_data(sfd,&head);
		FD_CLR(fileno(stdin),&readfds);
		FD_CLR(sfd,&readfds);
	}
}
