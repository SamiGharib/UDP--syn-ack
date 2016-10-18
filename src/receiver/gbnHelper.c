
#include "gbnHelper.h"

#define MAXWINDOWS 32
#define BUFSIZE 32

/*
 * fd : file descriptor where we're gonna write the output
 * sfd : socket file descritor
 * */
pkt_status_code selective_repeat(int fd, int sfd){

    pkt_t *buffer = malloc(sizeof(pkt_t)*BUFSIZE);
    int ret = -4, sel;
    uint8_t startBuffer = 0;
    uint8_t curSeqNum = 0;
    fd_set srfd;
    struct timeval tv;
    unsigned char buff[1024];
    uint16_t headBuffer[2];
    ssize_t readed;

    do {
        FD_ZERO(&srfd);
        FD_SET(fd, &srfd);
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        sel = select(fd+1, &srfd, NULL, NULL, &tv);
        if(sel > 0 && FD_ISSET(fd,&srfd)){
            memset(&buff, 0, 1024);
            readed = read(sfd, headBuffer, 4);
            size_t len = (size_t)ntohs(headBuffer[1]);
            if(readed != 8)
                return E_NOHEADER;
            //copying the head
            buff[0] = ((unsigned char*)headBuffer)[0];
            buff[1] = ((unsigned char*)headBuffer)[1];
            buff[2] = ((unsigned char*)headBuffer)[2];
            buff[3] = ((unsigned char*)headBuffer)[3];
            readed = read(sfd, buff+4, len+4);
            if(readed == -1)
                return E_UNCONSISTENT;
            pkt_t * pkt = pkt_new();
            len+=12;
            if(pkt_encode(pkt, buff, &len) != PKT_OK) {
                pkt_del(pkt);
                continue;
            }
            ret = treatPkt(&buffer, (uint8_t *)&startBuffer, &curSeqNum, pkt, fd);
            if(ret >= 0)
                sendACK(sfd, curSeqNum, pkt_get_timestamp(pkt));

        }

    }while(ret != 3);//mean an EOF

    free(buffer);

    return PKT_OK;
}

//TODO : check for ack
int treatPkt(pkt_t ** buffer, uint8_t *startBuf, uint8_t *curSeqNum, pkt_t * pkt, int fd){

    //try to insert it into the buffer
    uint8_t seqNum = pkt_get_seqnum(pkt);
    if((*curSeqNum) - seqNum < 0)return 1;//a packet who's late and duplicate
    if((*curSeqNum) - seqNum > 31)return -2;//a packet from the future
    if(buffer[((*startBuf) + ((*curSeqNum) - seqNum)) % 32] == NULL)
        buffer[((*startBuf) + ((*curSeqNum) - seqNum)) % 32] = pkt;
    else
        return 2;

    if(buffer[*startBuf] == NULL)
        return 0;

    //at this point, there is some data to extract from the buffer (consecutive frames)
    int i, size = 0;
    for(i = 0; i < BUFSIZE; i++)
        if((buffer[(*startBuf+i)%32]) != NULL)//counting the number of frame to output
            size++;
    int eof = 0;
    for(i = 0; i < size; i++) {
        if(pkt_get_length(buffer[(*startBuf)+i]) == 0 )
            eof =1;
        //outputting the packets
        ssize_t yolo = write(fd, pkt_get_payload(buffer[(*startBuf + i) % 32]), pkt_get_length(buffer[(*startBuf + i) % 32]));
        yolo ++;//TODO find another fix

        //freeing the packets
        pkt_del(buffer[(*startBuf + i) % 32]);

        //setting the buffer to NULL
        buffer[(*startBuf + i) % 32] = NULL;
    }
    //moving the current entry point of the buffer
    *startBuf = (uint8_t)((*startBuf+size) % 32);

    //moving the current seqnum
    *curSeqNum = (uint8_t)((*curSeqNum+size) % 256);

    if(eof)
        return 3;
    return 0;
}

int sendACK(int sfd, uint8_t curNumSeq, uint32_t timestamp){
    //creating the packet
    pkt_t *pkt = pkt_new();
    pkt_set_length(pkt, 0);
    pkt_set_type(pkt, PTYPE_ACK);
    pkt_set_seqnum(pkt, (const uint8_t) ((curNumSeq - 1)%32));
    pkt_set_window(pkt, MAXWINDOWS);
    pkt_set_timestamp(pkt, timestamp);
    //emit the packet here
    size_t len = 12 + pkt_get_length(pkt);
    unsigned char buff[len];
    pkt_encode(pkt, buff, &len);
    ssize_t ret = write(sfd, buff, len);
    pkt_del(pkt);
    if((size_t)ret != len)
        return -1;
    return 0;
}