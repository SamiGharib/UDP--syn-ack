#ifndef __HEADER_INCLUDE
#define __HEADER_INCLUDE
/* Network's include */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>

/* Classic include */
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/queue.h>

#include "../shared/packet_interface.h"
#include "../shared/real_address.h"
#include "../shared/create_socket.h"

#endif

#ifndef __SENDER_H_
#define __SENDER_H_
#ifndef __STRUCT_FRAME_
#define __STRUCT_FRAME_
struct pkt_time{
		pkt_t *pkt;
		struct timeval_t *tv;
}pkt_time_t;
#endif

#ifndef __CONST_SENDER_
#define __CONST_SENDER_
/* Time out time */
#define TIME_SEC 2
#define TIME_USEC 500000
/* Max time out -> 3*Max_Round_trip_time */
#define MAX_TIME_SEC 6
#define MAX_TIME_USEC 0
/* rate of adjustement of the time out */
#define DECREM_TIME_OUT 10 /* 10 percent */
#define INCREM_TIME_OUT 5 /* 20 percent */
#endif

/**
  * Method that send data to dest_addr by the port "port".
  * @pre addr != NULL && port > 0
  * @post return as soon as all the data has been correctly send. 
  *		This means that the last acknowledgment have to been received before
  *		closing the connection.
  */
int send_data(const char *dest_addr,int port);

pkt_t *prepare_packet(const uint8_t *data,uint16_t length);
#endif
