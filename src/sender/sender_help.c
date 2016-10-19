#include "sender_help.h"
#define DEBUG_HELP 0
int window_receiver = 1;
uint8_t next_seqnum = 0;
/* number of packet we can send */
int free_to_go = 1;
struct timeval RTT_MAX;
/* FIFO queue to stock pkt sent */
queue_t *head = NULL;
queue_t *tail = NULL;
/* Sigalrm variable */
sigjmp_buf env;
int end_of_data = 0;

/**
  * This function shall prepare a packet before sending it to the network.
  * @param data: Represent the payload of the segment (i.e. the data part if any).
  * @param length: The length of the payload. If the data isn't null, then the length should be > 0.
  * @param tv: This is a pointer to a struct timeval. It should be NULL, otherwise data pointed to by it will be erased.
  * @return This function return a pointer to the new created pkt_t structure. This structure is ready
  *			to be send over the socket after formating it via pkt_encode. Moreover, tv point to a struct timeval.
  */
pkt_t *prepare_packet(const uint8_t *data, uint16_t length, struct timeval **tv) {
    if (DEBUG_HELP) {
        fprintf(stderr, "Preparing a packet of length %d\n", length);
    }
    pkt_t *pkt = pkt_new();
    if (pkt == NULL)
        return NULL;
    if (pkt_set_type(pkt, PTYPE_DATA) != PKT_OK)
        return NULL;
    if (pkt_set_window(pkt, 0) != PKT_OK)
        return NULL;
    if (pkt_set_seqnum(pkt, next_seqnum) != PKT_OK)
        return NULL;
    if (next_seqnum == 255)
        next_seqnum = 0;
    else{
        next_seqnum++;
	}
   	*tv = (struct timeval *) malloc(sizeof(struct timeval));
    int err = gettimeofday(*tv, NULL);
    if (err != 0) {
        fprintf(stderr, "Error gettimeofday() for packet w/ seqnum %d\n", pkt_get_length(pkt));
        return NULL;
    }
    if (pkt_set_timestamp(pkt, (*tv)->tv_usec) != PKT_OK)
        return NULL;
    if (pkt_set_payload(pkt, data, length) != PKT_OK)
        return NULL;
    if (DEBUG_HELP) {
        fprintf(stderr, "Packet prepared : type : %d\n", pkt_get_type(pkt));
        fprintf(stderr, "window : %d (should be 0)\n", pkt_get_window(pkt));
        fprintf(stderr, "seqnum : %d\n", pkt_get_seqnum(pkt));
        fprintf(stderr, "length : %d\n", pkt_get_length(pkt));
    }
    return pkt;
}

/**
  * This function go through the queue and re-sent the packet that have timed out.
  * @param sfd: The file descriptor that represent the socket. It should be strictly greater than 0.
  * @return After the execution of this function, the packet that have timed out are resent (and thus, re-enqueue)
  */
void resend_data(int sfd) {
    struct timeval cmp;
    queue_t *itr = tail;
    while (itr != NULL) {
        if (DEBUG_HELP) {
            fprintf(stderr, "Start re-sending old packet\n");
        }
        struct timeval curr_time;
        int err = gettimeofday(&curr_time, NULL);
        if (err != 0) {
            fprintf(stderr, "Error w/ gettimeofday() while re-sending packet\n");
            return;
        }
        cmp.tv_sec = curr_time.tv_sec - (itr->tv)->tv_sec;
        cmp.tv_usec = curr_time.tv_usec - (itr->tv)->tv_usec;
        if (cmp.tv_sec > RTT_MAX.tv_sec || (cmp.tv_sec == RTT_MAX.tv_sec && cmp.tv_usec > RTT_MAX.tv_usec)) {
            RTT_MAX.tv_usec += (1000000 * cmp.tv_sec + cmp.tv_usec) / INCREM_TIME_OUT;
            if (RTT_MAX.tv_usec > 1000000) {
                RTT_MAX.tv_sec++;
                RTT_MAX.tv_usec = 0;
            }
            if (RTT_MAX.tv_sec > MAX_TIME_SEC || (RTT_MAX.tv_sec == MAX_TIME_SEC && RTT_MAX.tv_usec > MAX_TIME_USEC)) {
                RTT_MAX.tv_sec = MAX_TIME_SEC;
                RTT_MAX.tv_usec = MAX_TIME_USEC;
            }
            size_t pkt_length = pkt_get_length((itr->packet)) + 3 * sizeof(uint32_t);
            uint8_t buf[pkt_length];
            memset((void *) buf, 0, pkt_length);
            if (pkt_encode(itr->packet, buf, &pkt_length) != PKT_OK) {
                fprintf(stderr, "Error w/ pkt_encode() while re-sending packet\n");
                return;
            }
            ssize_t nBytes = send(sfd, (void *) buf, pkt_length, 0);
            if (DEBUG_HELP) {
                fprintf(stderr, "packet w/ seqnum %d resent\n", pkt_get_seqnum(itr->packet));
                fprintf(stderr, "New RTT_MAX %lld s and %lld us\n", (long long) RTT_MAX.tv_sec,
                        (long long) RTT_MAX.tv_usec);
            }
            if (nBytes == -1) {
                fprintf(stderr, "Error while re-sending packet over the socket : %s\n", strerror(errno));
                return;
            }
            free_to_go--;
            itr = tail->previous;
            queue_t *toEnqueue = dequeue(&head, &tail);
            re_enqueue(&head, &tail, toEnqueue);
        } 
		else
            return;
    }
	return;
}

/**
  * This function shall go trough the queue of waiting for re-sending packet and delete those
  * wich are well received by the receiver.
  * @param next_expected_seqnum: This is the next expected seqnum by the receiver.
  * @return -
  */
void acknowledge(uint16_t next_expected_seqnum) {
    if (DEBUG_HELP) {
			fprintf(stderr,"----------------------\n");
        fprintf(stderr, "Starting checking for packet w/ seqnum < %d\n", next_expected_seqnum);
    }
    queue_t *itr = head;
    while (itr != NULL) {
        if (pkt_get_seqnum(itr->packet) < next_expected_seqnum) {
            if (DEBUG_HELP) {
                fprintf(stderr, "Ack for packet w/ seqnum %d\n", pkt_get_seqnum(itr->packet));
            }
            struct timeval tmp;
            struct timeval curr_time;
            int err = gettimeofday(&curr_time, NULL);
            if (err != 0) {
                fprintf(stderr, "Error with gettimeofday to update RTT_MAX => no update will be performed\n");
            } else {
                tmp.tv_sec = curr_time.tv_sec - (itr->tv)->tv_sec;
                tmp.tv_usec = curr_time.tv_usec - (itr->tv)->tv_usec;
                if (DEBUG_HELP) {
                    fprintf(stderr, "Ack with %lld s and %lld us before TO\n", (long long) tmp.tv_sec,
                            (long long) tmp.tv_usec);
                }
                RTT_MAX.tv_usec -= (1000000 * tmp.tv_sec + tmp.tv_usec) / DECREM_TIME_OUT;
                if (RTT_MAX.tv_usec < 0) {
                    RTT_MAX.tv_sec--;
                    RTT_MAX.tv_usec = 999999;
                }
                if (RTT_MAX.tv_sec < 2) {
                    RTT_MAX.tv_sec = 2;
                    RTT_MAX.tv_usec = 0;
                }
                if (DEBUG_HELP) {
                    fprintf(stderr, "New RTT_MAX : %lld s and %lld us\n", (long long) RTT_MAX.tv_sec,
                            (long long) RTT_MAX.tv_usec);
                }
            }
            remove_queue(&head, &tail, itr);
            free_to_go++;
        }
        itr = itr->next;
    }
	if(DEBUG_HELP){
			fprintf(stderr,"----------------------\n");
	}
}

/**
  * Signal handler for the alarm signal (used when window of the last received packet is 0
  */
static void sigalrm_handler(int signum) {
    siglongjmp(env, 1);
}

/**
  * This function shall read stdin until EOF encounterd. All data read on stdin are sent by the socket to dest_addr.
  * Once EOF is encountered, the function shall start the deconnection routine.
  * @param dest_addr: The destination of the data. It can be of the form "www.mysupersite.com" for example. It can't be NULL
  * @param port: The port by wich the data will be sent. It must be > 0.
  * @return If all the data has been sent without problem, then this function return 0. If an error has occured, the -1 is return
  *			and an error message is sent to stderr
  */
int send_data(const char *dest_addr,int port){
		/* Setting sig_handler for alarm */
		if(signal(SIGALRM,sigalrm_handler) == SIG_ERR){
				fprintf(stderr,"Error while setting sig handler for the alarm signal : %s\n",strerror(errno));
				return -1;
		}
		/* Setting the max RTT_MAX */
		RTT_MAX.tv_sec = TIME_SEC;
		RTT_MAX.tv_usec = TIME_USEC;
		/* Getting the real address */
		struct sockaddr_in6 addr;
		const char *err = real_address(dest_addr,&addr);
		if(err){
				fprintf(stderr,"Error while resolving hostname %s: %s\n",dest_addr,err);
				return -1;
		}
		if(DEBUG_HELP){
				fprintf(stderr,"Address : %s\n",addr.sin6_addr.s6_addr);
		}
		/* Getting the socket */
		int sfd = create_socket(NULL,-1,&addr,port);
		if(sfd < 0){
				fprintf(stderr,"Failed to create the socket\n");
				return -1;
		}
		if(DEBUG_HELP){
				fprintf(stderr,"Socket created -> %d\n",sfd);
		}
		/* Start to read data */
		/* Buffer to store payload */
		uint8_t write_buf[512];
		uint8_t read_buf[512];
		memset((void *)write_buf,0,MAX_PAYLOAD_SIZE);
		memset((void *)read_buf,0,MAX_PAYLOAD_SIZE);
		/* Buffer to store packet */
		uint8_t toSend[MAX_PACKET_SIZE];
		uint8_t toReceive[MAX_PACKET_SIZE];
		uint8_t last_sent[MAX_PACKET_SIZE];
		memset((void *)toSend,0,MAX_PACKET_SIZE);
		memset((void *)toReceive,0,MAX_PACKET_SIZE);
		ssize_t last_sent_length=0;
		/* Sets for the select() call */
		fd_set readfds;
		fd_set writefds;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		ssize_t nBytes;
		while(1){
				/* TERMINATED THE CONNECTION */
				if(end_of_data && head == NULL && tail == NULL){
						if(DEBUG_HELP){
								fprintf(stderr,"End of data -> sending end-of-communication segment\n");
						}
						/* SEGFAULT ICI BB */
						struct timeval *tv=NULL;
						pkt_t *pkt = prepare_packet(NULL,0,&tv);
						if(pkt == NULL){
								fprintf(stderr,"Error, not enough memory to send the end-of-communication segment\n");
								return -1;
						}
						size_t len = MAX_PACKET_SIZE;
						pkt_encode(pkt,toSend,&len);
						nBytes = send(sfd,(void *)toSend,len*sizeof(uint8_t),0);
						if(nBytes == -1){
								fprintf(stderr,"Error while sending the end-of-communication segment : %s\n Terminating the connection brutally\n",strerror(errno));
								return -1;
						}
						return 0;
				}
				FD_SET(fileno(stdin),&readfds);
				FD_SET(sfd,&readfds);
				FD_SET(sfd,&writefds);
				int rv = select(sfd+1,&readfds,&writefds,NULL,NULL);
				if(rv == -1){
						fprintf(stderr,"Error while multiplexing stdin and the socket %d : %s\n",sfd,strerror(errno));
						return -1;
				}
				/* Receiving ACK */
				if(FD_ISSET(sfd,&readfds)){
						if(DEBUG_HELP){
								fprintf(stderr,"Receiving ack\n");
						}
						nBytes = recv(sfd,(void *)toReceive,3*sizeof(uint32_t),0);
						if(nBytes == -1){
								fprintf(stderr,"Error while receiving ack : %s\n",strerror(errno));
								return -1;
						}
						pkt_t *pkt = pkt_new();
						pkt_status_code ret =pkt_decode(toReceive,nBytes*sizeof(uint8_t),pkt);
						if(DEBUG_HELP){
								fprintf(stderr,"ret code for pkt_decode : %d\n",ret);
						}
						if(ret== PKT_OK){
							if(DEBUG_HELP){
									fprintf(stderr,"seqnum of the ack : %d\n",pkt_get_seqnum(pkt));
									fprintf(stderr,"window of the ack : %d\n",pkt_get_window(pkt));
							}
								if(DEBUG_HELP){
										fprintf(stderr,"Should begin ack function\n");
								}
								acknowledge(pkt_get_seqnum(pkt));
								if(free_to_go > pkt_get_window(pkt))
										free_to_go = pkt_get_window(pkt);
								else
										free_to_go += pkt_get_window(pkt) - free_to_go;
								if(pkt_get_window(pkt) == 0){
										if(sigsetjmp(env,1)==0){
												alarm(4);
										}
										else{
												send(sfd,last_sent,last_sent_length,0);
												alarm(4);
										}
								}
								else
										alarm(0);
						}
				}
				/* Ready to send data */
				if(FD_ISSET(fileno(stdin),&readfds) && FD_ISSET(sfd,&writefds) && free_to_go != 0 && end_of_data == 0){
						/* read data from stdin */
						nBytes = read(fileno(stdin),(char *)write_buf,MAX_PAYLOAD_SIZE);
						/* EOF => cut connection */
						if(nBytes == 0){
								fprintf(stderr,"Receiving EOF -> will check if any segment not ack\n");
								end_of_data=1;
						}
						else if(nBytes == -1){
								fprintf(stderr,"Error while reading stdin : %s\n",strerror(errno));
								return -1;
						}
						else{
							/* Prepare packet and sending it by the socket */
							memcpy((void *)last_sent,(void *)write_buf,nBytes);
							last_sent_length = nBytes;
							struct timeval *tv=NULL;
							pkt_t *pkt = prepare_packet(write_buf,(uint16_t)nBytes,&tv);
							if(pkt != NULL){
									size_t len = MAX_PACKET_SIZE;
									pkt_encode(pkt,toSend,&len);
									if(DEBUG_HELP){
											fprintf(stderr,"Sending packet w/ seqnum %d w/ length %d\n",pkt_get_seqnum(pkt),pkt_get_length(pkt));
									}
									send(sfd,toSend,len*sizeof(uint8_t),0);
									memset((void *)last_sent,0,MAX_PACKET_SIZE);
									memcpy((void *)last_sent,(void *)toSend,len*sizeof(uint8_t));
									last_sent_length = len*sizeof(uint8_t);
									if(enqueue(&head,&tail,pkt,tv) == NULL){
											fprintf(stderr,"No memory to store packet sent (for eventual re-sending). Terminating.\n");
											return -1;
									}
									free_to_go--;
									if(DEBUG_HELP){
											fprintf(stderr,"free to go : %d\n",free_to_go);
									}
							}
						}
				}
				if(free_to_go != 0)
					resend_data(sfd);
				FD_CLR(fileno(stdin),&readfds);
				FD_CLR(sfd,&readfds);
				FD_CLR(sfd,&writefds);
		}
		return 0;
}

