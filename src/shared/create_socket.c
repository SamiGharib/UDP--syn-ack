#include <string.h>
#include <stdio.h>

/* Network include */
#include <sys/socket.h>

#include "create_socket.h"

int create_socket(struct sockaddr_in6 *source_addr,
				int src_port,
				struct sockaddr_in6 *dest_addr,
				int dst_port){
		int sd = socket(AF_INET6,SOCK_DGRAM,IPPROTO_UDP);
		if(sd == -1)
				return -1;
		int err;
		if(source_addr == NULL){
				dest_addr->sin6_port = htons(dst_port);
				err= connect(sd,(struct sockaddr *)dest_addr,sizeof(struct sockaddr_in6));
		}
		else{
				source_addr->sin6_port =htons(src_port);
				err = bind(sd,(struct sockaddr *)source_addr,sizeof(struct sockaddr_in6));
		}
		if(err == -1)
				return -1;
		return sd;
}
