#include "sender_help.h"
#include "packet_interface.h"

int window_receiver=1;
uint8_t next_seqnum=0;
/* number of packet we can send */
int free_to_go=1;
struct timeval RTT;
/* FIFO queue to stock pkt sent */
queue_t *head = NULL;
queue_t *tail = NULL;

/**
  * Prepare a packet before sending it to the netork
  * @pre 0 < window < 31 && 0 < length < 512
  * @post return a pointer to the new struct pkt created. If an
  *			error occured, return NULL. The seqnum is incremented
  *			for the next packet.
  */
pkt_t * prepare_packet(const uint8_t *data,uint8_t window,uint16_t length){
		pkt_t *pkt = pkt_new();
		if(pkt == NULL)
				return NULL;
		if(pkt_set_type(pkt,PTYPE_DATA) != PKT_OK)
				return NULL;
		if(pkt_set_window(pkt,window) != PKT_OK)
				return NULL;
		if(pkt_set_seqnum(pkt,next_seqnum) != PKT_OK)
				return NULL;
		next_seqnum++;
		struct timeval *tv = (struct timeval *)malloc(sizeof(struct timeval));
		int err = gettimeofday(tv,NULL);
		if(err != 0){
				fprintf(stderr,"Error gettimeofday() for packet w/ seqnum %d\n",pkt_get_length(pkt));
				return NULL;
		}
		if(pkt_set_timestamp(pkt,tv->tv_usec) != PKT_OK)
				return NULL;
		if(pkt_set_payload(pkt,data,length) != PKT_OK)
				return NULL;
		return pkt;
}

/**
  * Go trough the buffer with the stocked packet and resend
  * those that have timed out
  * @pre sfd > 0
  * @post the packet that have timed out have been received. The limit
  *			of packet that can be resend is free_to_go
  */
void resend_packet(int sfd,queue_t *head,queue_t *tail){
		size_t length = MAX_PACKET_SIZE;
		uint8_t toSend[length];
		memset((void *)toSend,0,length);
		ssize_t nBytes;
		int i;
		while(tail != NULL){
				struct timeval cmp;
				struct timeval tv;
				int err = gettimeofday(&tv,NULL);
				if(err == -1){
						fprintf(stderr,"Error gettimeofday while re-sending packet\n");
						return;
				}
					struct timeval *pkt_tv= tail->tv;
					cmp.tv_sec = tv.tv_sec - pkt_tv->tv_sec;
					cmp.tv_usec = tv.tv_usec - pkt_tv->tv_usec; 
					if(cmp.tv_sec > RTT.tv_sec || (cmp.tv_sec == RTT.tv_sec && cmp.tv_usec > RTT.tv_usec)){
							queue_t *old_tail = dequeue(head,tail);
							if(pkt_encode(old_tail->packet,toSend,&length) == PKT_OK){
									nBytes = send(sfd,(void *)toSend,length,0);
									if(nBytes == -1){
											fprintf(stderr,"Error while re-sending packet with seqnum %d : %s\n",pkt_get_seqnum(buf[i]),strerror(errno));
											return;
									}
							}
							re_enqueue(head,tail,old_tail);
					}
					memset((void *)toSend,0,MAX_PACKET_SIZE);
		}
}

void

int send_data(const char *dest_addr,int port){
		RTT.tv_sec = 2;
		RTT.tv_usec = 0;
		/* Buffer to store packet if need to re-send */
		pkt_t * buffer_packet[16];
		uint8_t buffer_place = 16;
		/* Getting the real address */
		struct sockaddr_in6 addr;
		const char *err = real_address(dest_addr,&addr);
		if(err){
				fprintf(stderr,"Error while resolving hostname %s: %s\n",dest_addr,err);
				return -1;
		}
		/* Getting the socket */
		int sfd = create_socket(NULL,-1,&addr,port);
		if(sfd < 0){
				fprintf(stderr,"Failed to create the socket\n");
				return -1;
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
		memset((void *)toSend,0,MAX_PACKET_SIZE);
		memset((void *)toReceive,0,MAX_PACKET_SIZE);
		/* Sets for the select() call */
		fd_set readfds;
		fd_set writefds;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		ssize_t nBytes;
		while(1){
				FD_SET(fileno(stdin),&readfds);
				FD_SET(sfd,&readfds);
				FD_SET(sfd,&writefds);
				int rv = select(sfd+1,&readfds,&writefds,NULL,NULL);
				if(rv == -1){
						fprintf(stderr,"Error while multiplexing stdin and the socket %d : %s\n",sfd,strerror(errno));
						return -1;
				}
				if(FD_ISSET(sfd,&readfds)){
						/* Receiving ack */
				}
				/* Ready to send data */
				if(FD_ISSET(fileno(stdin),&readfds) && FD_ISSET(sfd,&writefds) && free_to_go != 0){
						/* read data from stdin */
						nBytes = read(fileno(stdin),(char *)write_buf,MAX_PAYLOAD_SIZE);
						/* EOF => cut connection */
						if(nBytes == 0){
						}
						if(nBytes == -1){
								fprintf(stderr,"Error while reading stdin : %s\n",strerror(errno));
								return -1;
						}
						/* Prepare packet and sending it by the socket */
						pkt_t *pkt = prepare_packet(write_buf,buffer_place,(uint16_t)nBytes);
						if(pkt != NULL){
								size_t len = MAX_PACKET_SIZE;
								pkt_encode(pkt,toSend,&len);
								send(sfd,toSend,len*sizeof(uint8_t),0);
								if(enqueue(head,tail,pkt,pkt->tv) == NULL)
										/* NO MEMORY => BUFFER = 0? */
								free_to_go--;
						}
				}
				resend_data(sfd,head,tail);
				FD_CLR(fileno(stdin),&readfds);
				FD_CLR(sfd,&readfds);
				FD_CLR(sfd,&writefds);
		}
		return 0;
}

