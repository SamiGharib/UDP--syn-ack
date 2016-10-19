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

/* Extra code */
/* Your code will be inserted here */

pkt_t* pkt_new()
{
		return calloc(1,sizeof(pkt_t));
}

void pkt_del(pkt_t *pkt)
{
		if(pkt != NULL){
				free(pkt->payload);
				free(pkt);
		}
}

pkt_status_code pkt_decode(const uint8_t *data, const size_t len, pkt_t *pkt)
{
		if(len < 12)
				return E_UNCONSISTENT;
		uLong crc_check;
		memcpy(((void *)pkt),(void *)data,2*sizeof(uint32_t));
		if(pkt_get_type(pkt) == PTYPES_DATA){
			pkt->length = ntohs(pkt->length);
			uint8_t *payload = (uint8_t *)malloc(pkt->length*sizeof(uint8_t));
			memcpy((void *)payload,((void *)data)+2*sizeof(uint32_t),pkt->length*sizeof(uint8_t));
			pkt->payload = payload;
			memcpy((void *)&(pkt->crc),((void *)data)+2*sizeof(uint32_t)+pkt->length*sizeof(uint8_t),sizeof(uint32_t));
			pkt->crc = ntohl(pkt->crc);
			check_crc = crc32(0L,(Bytef *)data,2*sizeof(uint32_t)+pkt->length*sizeof(uint8_t));
			if(check_crc != pkt->crc)
					return E_CRC;
		}
		else{
				pkt->length = 0;
				memcpy(((void *)pkt)+2*sizeof(uint32_t),((void *)data)+2*sizeof(uint32_t),sizeof(uint32_t));
				check_crc = crc32(0L,(Bytef *)data,3*sizeof(uint32_t));
				if(check_crc != pkt->crc)
						return E_CRC;
		}
		return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, uint8_t *buf, size_t *len)
{
		if(*len < pkt->length*sizeof(uint8_t)+3*sizeof(uint32_t))
				return E_NOMEM;
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

const uint8_t* pkt_get_payload(const pkt_t* pkt)
{
		return (uint8_t *)pkt->payload;
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
							    const uint8_t *data,
								const uint16_t length)
{
		pkt_status_code ret_code;
		if(length > MAX_PAYLOAD_SIZE)
				return ret_code=E_LENGTH;
		if(data == NULL)
				return PKT_OK;
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

