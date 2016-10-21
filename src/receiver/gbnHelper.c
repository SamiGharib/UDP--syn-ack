
#include "gbnHelper.h"

#define MAXWINDOWS 32
#define BUFSIZE 32

/*
 * fd : file descriptor where we're gonna write the output
 * sfd : socket file descritor
 * */
pkt_status_code selective_repeat(int fd, int sfd){

    pkt_t **buffer = (pkt_t**)calloc(sizeof(pkt_t*),BUFSIZE);
    int ret = -4, sel;
    uint8_t startBuffer = 0;
    uint8_t curSeqNum = 0;
    fd_set srfd;
    struct timeval tv;
    unsigned char buff[MAX_PACKET_SIZE];
    ssize_t readed;
    memset(&buff, 0, MAX_PACKET_SIZE);

    do {
        FD_ZERO(&srfd);
        FD_SET(sfd, &srfd);
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        sel = select(sfd+1, &srfd, NULL, NULL, &tv);
        if(sel > 0 && FD_ISSET(sfd,&srfd)){
            readed = read(sfd, buff, MAX_PACKET_SIZE);
            if(readed == -1) {
                free(buffer);
                return E_UNCONSISTENT;
            }

            pkt_t * pkt = pkt_new();

            if(pkt_decode(buff, MAX_PACKET_SIZE, pkt) != PKT_OK) {
                pkt_del(pkt);
                continue;
            }
            ret = treatPkt(buffer, &startBuffer, &curSeqNum, pkt, fd);

            if(ret >= 0)
                sendACK(sfd, curSeqNum, pkt_get_timestamp(pkt));

        }

    }while(ret != 3);//mean an EOF

    free(buffer);

    return PKT_OK;
}

int treatPkt(pkt_t ** buffer, uint8_t *startBuf, uint8_t *curSeqNum, pkt_t * pkt, int fd){

    uint8_t seqNum = pkt_get_seqnum(pkt);
    if(seqNum - (*curSeqNum) < 0)return 1;//a packet who's late and duplicate
    //test to see if  there is a packet who's waaay after the current windows indicating that the sender don't care about the ack
    if(seqNum - (*curSeqNum) - BUFSIZE*1.5 > BUFSIZE-1){
        fprintf(stderr, "Le sender ne tient pas compte de la windows !\n");
        exit(-1);
    }
    if(seqNum - (*curSeqNum) > BUFSIZE-1)return -2;//a packet from the future
    //TODO add a buff

    if(buffer[((*startBuf) + (seqNum - (*curSeqNum))) % BUFSIZE] == NULL)
        buffer[((*startBuf) + (seqNum - (*curSeqNum))) % BUFSIZE] = pkt;
    else
        return 2;

    if(buffer[*startBuf] == NULL)//the current first packet is missing, skipping
        return 0;

    //at this point, there is some data to extract from the buffer (consecutive frames)
    int i, size = 0;
    for(i = 0; i < BUFSIZE; i++)
        if((buffer[(*startBuf+i)%BUFSIZE]) != NULL)//counting the number of frame to output
            size++;

    int eof = 0;
    for(i = 0; i < size; i++) {

        if(pkt_get_length(buffer[((*startBuf)+i)%BUFSIZE]) == 0 )
            eof =1;
        //outputting the packets
        ssize_t yolo = write(fd, pkt_get_payload(buffer[(*startBuf + i) % BUFSIZE]), pkt_get_length(buffer[(*startBuf + i) % BUFSIZE]));
        yolo ++;//TODO find another fix

        //freeing the packets
        pkt_del(buffer[(*startBuf + i) % 32]);

        //setting the buffer to NULL
        buffer[(*startBuf + i) % 32] = NULL;
    }
    //moving the current entry point of the buffer
    *startBuf = (uint8_t)((*startBuf+size) % BUFSIZE);

    //moving the current seqnum
    *curSeqNum = (uint8_t)((*curSeqNum+size) % 256);

    if(eof)
        return 3;
    return 0;
}

int sendACK(int sfd, uint8_t curNumSeq, uint32_t timestamp){
    //TODO: add the remaining free emplacement into the windows
    //creating the packet
    pkt_t *pkt = pkt_new();
    pkt_set_length(pkt, 0);
    pkt_set_type(pkt, PTYPE_ACK);
    pkt_set_seqnum(pkt, (const uint8_t) ((curNumSeq)%256));
    pkt_set_window(pkt, MAXWINDOWS-1);
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