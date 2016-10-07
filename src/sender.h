#ifndef __SENDER_H_
#define __SENDER_H_
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
/**
  * Method that send data to dest_addr by the port "port".
  * @pre addr != NULL && port > 0
  * @post return as soon as all the data has been correctly send. 
  *		This means that the last acknowledgment have to been received before
  *		closing the connection.
  */
void send(const char *dest_addr,int port);

#endif
