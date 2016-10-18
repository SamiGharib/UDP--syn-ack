#include "packet_interface.h"

/* Extra #includes */
#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fcntl.h>
/* Your code will be inserted here */

struct __attribute__((__packed__)) pkt {
	uint32_t window:5,
			 type:3,
			 seqnum:8,
			 length:16;
	uint32_t timestamp;
	uint8_t *payload;
	uint32_t crc;
};

/* Extra code */
/* Your code will be inserted here */

pkt_t* pkt_new()
{
		pkt_t *pkt =(pkt_t *)malloc(sizeof(pkt_t));
		if(pkt == NULL)
				return NULL;
		pkt->payload = NULL;
		return pkt;
}

void pkt_del(pkt_t *pkt)
{
		if(pkt != NULL){
				free(pkt->payload);
				free(pkt);
		}
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{	
		if(len < 12)
				return E_UNCONSISTENT;
		memcpy(((void *)pkt),(void *)data,2*sizeof(uint32_t));
		pkt->length = ntohs(pkt->length);
		uint8_t *payload = (uint8_t *)malloc(pkt->length*sizeof(uint8_t));
		memcpy((void *)payload,((void *)data)+2*sizeof(uint32_t),pkt->length*sizeof(uint8_t));
		pkt->payload = payload;
		memcpy((void *)&(pkt->crc),((void *)data)+2*sizeof(uint32_t)+pkt->length*sizeof(uint8_t),sizeof(uint32_t));
		pkt->crc = ntohl(pkt->crc);
		uLong check_crc = crc32(0L,(Bytef *)data,2*sizeof(uint32_t)+pkt->length*sizeof(uint8_t));
		if(check_crc != pkt->crc)
				return E_CRC;
		return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
		memset((void *)buf,0,*len);
		pkt_t pkt_tmp = *pkt;
		uint16_t payload_size = pkt_get_length(pkt);
		/* Converting field into Network Byte order */
		pkt_tmp.length = htons(pkt_tmp.length);
		/* Writing to buffer */
		memcpy((void *)buf,(void *)&pkt_tmp,2*sizeof(uint32_t));
		memcpy(((void *)buf)+2*sizeof(uint32_t),(void *)pkt_tmp.payload,payload_size);
		/* Calculate CRC */
		uLong crc = crc32(0L,(Bytef *)buf,2*sizeof(uint32_t)+payload_size);
		pkt_tmp.crc = htonl(crc);
		memcpy(((void *)buf)+2*sizeof(uint32_t)+payload_size,(void *)&pkt_tmp.crc,4);
		*len = 8+payload_size+4;
		return PKT_OK;

}

ptypes_t pkt_get_type  (const pkt_t* pkt)
{
		return pkt->type;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
		return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
		return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
		return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
		return pkt->timestamp;
}

uint32_t pkt_get_crc   (const pkt_t* pkt)
{
		return pkt->crc;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
		return (char *)pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
		pkt_status_code ret_code;
		pkt->type = type;
		return ret_code=PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
		pkt_status_code ret_code;
		if(window > MAX_WINDOW_SIZE)
				return ret_code=E_WINDOW;
		pkt->window = window;
		return ret_code=PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
		pkt_status_code ret_code;
		pkt->seqnum = seqnum;
		return ret_code=PKT_OK;

}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
		pkt_status_code ret_code;
		if(length > MAX_PAYLOAD_SIZE)
				return ret_code=E_LENGTH;
		pkt->length = length;
		return ret_code=PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
		pkt_status_code ret_code;
		pkt->timestamp = timestamp;
		return ret_code=PKT_OK;
}

pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc)
{
		pkt_status_code ret_code;
		pkt->crc = crc;
		return ret_code=PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
							    const char *data,
								const uint16_t length)
{
		pkt_status_code ret_code;
		if(length > MAX_PAYLOAD_SIZE)
				return ret_code=E_LENGTH;
		if(pkt->payload == NULL){
				pkt->payload = (uint8_t *)malloc(length*sizeof(uint8_t));
				if(pkt->payload == NULL)
						return ret_code = E_NOMEM;
		}
		else{
			void *ptr = realloc((void *)pkt->payload,length*sizeof(uint8_t));
				memset((void *)ptr,0,length*sizeof(uint8_t));
				pkt->payload = (uint8_t *)ptr;
		}
		memcpy((void *)pkt->payload,(void *)data,length);
		ret_code = pkt_set_length(pkt,length);
		return ret_code;
}

int main(void){
		pkt_t *pkt = pkt_new();
		pkt_set_type(pkt,1);
		pkt_set_window(pkt,15);
		pkt_set_seqnum(pkt,123);
		pkt_set_timestamp(pkt,10);
		int fd = open("payload",O_RDONLY);
		if(fd == -1){
				fprintf(stderr,"Error opening file payload\n");
				return -1;
		}
		uint8_t *buf =(uint8_t *)malloc(11*sizeof(uint8_t));
		read(fd,(void *)buf,11);
		pkt_set_payload(pkt,(char *)buf,11);
		char *buf2 = (char *)malloc(3*sizeof(uint32_t)+11*sizeof(uint8_t));
		size_t length = 3*sizeof(uint32_t)+11*sizeof(uint8_t);
		pkt_status_code ret_code = pkt_encode(pkt,buf2,&length);
		int fd_out = open("output",O_WRONLY | O_CREAT);
		write(fd_out,(void *)buf2,3*sizeof(uint32_t)+11*sizeof(uint8_t));
		close(fd);
		close(fd_out);
		return ret_code;
}
