#ifndef __RECEIVER_H_
#define __RECEIVER_H_
#ifndef __STRUCT_FRAME_
#define __STRUCT_FRAME_
struct frame{
		uint32_t type:3,
				 window:5,
				 seqnum:8,
				 length,16;
		uint32_t timestamp;
		uint8_t *payload;
		uint32_t crc;
};
#endif
/** Receive data from src_addr and display it on stdout 
  *	(or write it file if specified by the user)
  * @pre src_addr != NULL && port > 0
  * @post return as soon as the connection is lost with the source
  */
void receiver(const char *src_addr,int port);

#endif
