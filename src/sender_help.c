#include "sender.h"
#include "packet_interface.h"

/* Index for the windows */
int window_receiver=1;

int send_data(const char *dest_addr,int port){
		/* Buffer to store packet if need to re-send */
		pkt * buffer_packet[16];
		/* Getting the real address */
		struct sockaddr_in6 addr;
		const char *err = real_address(dest_addr,&addr);
		if(err){
				fprintf(stderr,"Error while resolving hostname %s: %s\n",host,err);
				return EXIT_FAILURE;
		}
		/* Getting the socket */
		int sfd = create_socket(NULL,-1,&addr,port);
		if(sfd < 0){
				fprintf(stderr,"Failed to create the socket\n");
				return EXIT_FAILURE;
		}
		/* Start to read data */
		uint8_t write_buf[512];
		uint8_t read_buf[512];
		/* BUF_LENGTH defined in sender.h such that BUF_LENGTH = 512*sizeof(uint8_t) */
		memset((void *)write_buf,0,MAX_PAYLOAD_SIZE);
		memset((void *)read_buf,0,MAX_PAYLOAD_SIZE);
		/* Sets for the select() call */
		fd_set readfds;
		fd_set writefds;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		while(1){
				FD_SET(fileno(stdin),&readfds);
				FD_SET(sfd,&readfds);
				FD_SET(sfd,&writefds);
				int rv = select(sfd+1,&readfds,&writefds,NULL,NULL);
				if(rv == -1){
						fprintf(stderr,"Error while multiplexing stdin and the socket %d : %s\n",sfd,strerror(errno));
						return EXIT_FAILURE;
				}
				if(FD_ISSET(sfd,&readfds)){
						/* Receiving ack */
				}
				if(FD_ISSET(fileno(stdin),&readfds) && FD_ISSET(sfd,&writefds)){
						/* Sending data */
				}
				FD_CLR(fileno(stdin),&readfds);
				FD_CLR(sfd,&readfds);
				FD_CLR(sfd,&writefds);
		}
}

