#include <string.h>
#include <stdio.h>

/* Network include's */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "real_address.h"

const char *real_address(const char *address,struct sockaddr_in6 *rval){
		struct addrinfo hints;
		struct addrinfo *result;
		/* Setting the memory to 0 before initializing fields */
		memset(&hints,0,sizeof(struct addrinfo));

		hints.ai_family = AF_INET6; /* Allow only IPV6 address */
		hints.ai_socktype= SOCK_DGRAM; /* Dram socket */
		hints.ai_flags = AI_PASSIVE; /* suitable for bind() and connect() */
		int err = getaddrinfo(address,NULL,&hints,&result);
		if(err != 0)
				return gai_strerror(err);
		memcpy(rval,result->ai_addr,sizeof(struct sockaddr_in6));
		freeaddrinfo(result);
		return NULL;
}
