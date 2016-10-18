#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "wait_for_client.h"

int wait_for_client(int sfd){
	char buf [1024];
	struct sockaddr_in6 st;
	socklen_t slt = sizeof(struct sockaddr_in6);
	memset(buf, 0, 128);
	memset(&st, 0,sizeof(struct sockaddr_in6));

	int co = recvfrom(sfd, buf, 1024, MSG_PEEK, (struct sockaddr *)&st, &slt);
	if(co == -1)return -1;
	//connect here
	return connect(sfd, (const struct sockaddr *)&st, slt);
}
