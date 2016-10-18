#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <netinet/in.h>
#include <zlib.h>
#include "packet_interface.h"
#include <inttypes.h>

/* Extra #includes */
/* Your code will be inserted here */


//return the crcr at the host format
uint32_t getCRC(const pkt_t * pkt){
	unsigned long int crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, (unsigned char*)pkt, 8);
    crc = crc32(crc, (unsigned char*)(pkt->data), pkt_get_length(pkt));
	return (uint32_t)crc;
}

/* Extra code */
/* Your code will be inserted here */

pkt_t* pkt_new()
{
    pkt_t * pkt = (pkt_t*)malloc(sizeof(pkt_t));
    if(pkt == NULL)return NULL;
    memset(pkt,0,sizeof(pkt_t));
    return pkt;
}

//the payload must already have been free
void pkt_del(pkt_t *pkt)
{
    //free the payload here of it exist
    free(pkt);
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
    if(pkt == NULL)//checking if the initial allocation is ok
        return E_NOMEM;

    pkt_status_code status;

    memcpy(pkt, data, 8);

        //tests of corectness of the paquet

    if(pkt_get_length(pkt)+12 != (uint16_t)len)//we're checking if the given number of byte is equal to the length computed from the internal length
        return E_UNCONSISTENT;

    if(pkt_get_length(pkt) > 512)//check for the maxlength
        return E_LENGTH;

    if(pkt_get_type(pkt) != PTYPE_ACK && pkt_get_type(pkt) != PTYPE_DATA)//check for the validity of the type
        return E_TYPE;

        //crc work !
    status = pkt_set_payload(pkt, data+8,pkt_get_length(pkt));//setting the payload
    if(status != PKT_OK)return status;

    pkt_set_crc(pkt, ntohl(*(uint32_t*)(data+8+pkt_get_length(pkt))));//setting the crc
    if(getCRC(pkt)!= pkt_get_crc(pkt))return E_CRC;//the packet is faulty, returning the error

    return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
    if(pkt == NULL)return E_UNCONSISTENT;
    if(*len < 12)return E_NOHEADER;
    if(*len < (size_t)12+pkt_get_length(pkt))return E_NOMEM;
    *len = 0;
    uint8_t *header = (uint8_t*)pkt;
    memcpy(buf, header, 8);//dest src byte
    if(pkt_get_length(pkt)!=0)
        memcpy(buf+8, pkt_get_payload(pkt), pkt_get_length(pkt));//TODO manage the null case
    *len += 12+pkt_get_length(pkt);

    uint32_t yoloCRC = htonl(getCRC(pkt));
    memcpy(buf+8+pkt_get_length(pkt), &yoloCRC, 4);
    return PKT_OK;
}

ptypes_t pkt_get_type  (const pkt_t* pkt)
{
    return pkt->type;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
    return pkt->windows;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
    return ntohs(pkt->length);
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
    return pkt->timestamp;
}

uint32_t pkt_get_crc   (const pkt_t* pkt)
{
    return ntohl(pkt->crc);
}

const char* pkt_get_payload(const pkt_t* pkt)//TODO return a copy of the payload here not the actual payload
{
    if(pkt_get_length(pkt)==0)return NULL;
    return pkt->data;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
    if(type != PTYPE_DATA && type != PTYPE_ACK)return E_TYPE;
    pkt->type = type;
    return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
    pkt->windows = window;
    return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
    pkt->seqnum = seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
    if(length > 512)return E_LENGTH;
    pkt->length = htons(length);
    return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
    if(timestamp <= 0)return E_UNCONSISTENT;
    pkt->timestamp = timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc)
{
	//recompute crc here
    pkt->crc = htonl(crc);
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
                                const char *data,
                                const uint16_t length)
{
    if(pkt->data != NULL)//TODO replace that with a realloc
        free(pkt->data);
    char * pt = (char*)malloc(sizeof(char)*length);
    if(pt == NULL)return E_NOMEM;
    memcpy(pt,data,length);
    pkt_set_length(pkt,length);
    pkt->data = pt;
    return PKT_OK;
}
