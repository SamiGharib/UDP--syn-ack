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

#include "../shared/packet_interface.h"
#include "../shared/real_address.h"
#include "../shared/create_socket.h"
#endif

#ifndef __SENDER_H_
#define __SENDER_H_
#ifndef __STRUCT_FRAME_
#define __STRUCT_FRAME_
struct frame{
		uint32_t type:3,
				 window:5,
				 seqnum:8,
				 length:16;
		uint32_t timestamp;
		uint8_t *payload;
		uint32_t crc;
};
#endif
/**
  * Method that send data to dest_addr by the port "port".
  * @pre addr != NULL && port > 0
  * @post return as soon as all the data has been correctly send. 
  *		This means that the last acknowledgment have to been received before
  *		closing the connection.
  */
int send_data(const char *dest_addr,int port);

pkt_t *prepare_packet(const uint8_t *data,uint8_t window,uint16_t length);
#endif
