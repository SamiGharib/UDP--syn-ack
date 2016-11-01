
#include "gbnHelper.h"

#define BUFSIZE 32
#define ISDBG 1

/*
 * fd : file descriptor where we're gonna write the output
 * sfd : socket file descritor
 * */
pkt_status_code selective_repeat(int fd, int sfd){

    pkt_t **buffer = (pkt_t**)calloc(sizeof(pkt_t*),BUFSIZE);
    int ret = -4, sel, disp = BUFSIZE-1;
    uint8_t startBuffer = 0, curSeqNum = 0;
    fd_set srfd;
    struct timeval tv;
    unsigned char buff[MAX_DATA_PACKET_SIZE];
    ssize_t readed;
    pkt_t *pkt = NULL;
    memset(&buff, 0, MAX_DATA_PACKET_SIZE);
    uint32_t timestamp = 0;

    do {
        FD_ZERO(&srfd);
        FD_SET(sfd, &srfd);
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        sel = select(sfd+1, &srfd, NULL, NULL, &tv);
        if(sel > 0 && FD_ISSET(sfd,&srfd)){
            readed = read(sfd, buff, MAX_DATA_PACKET_SIZE);
            if(readed == -1) {
                free(buffer);
                return E_UNCONSISTENT;
            }

            pkt = pkt_new();

            if(pkt_decode(buff, MAX_DATA_PACKET_SIZE, pkt) != PKT_OK) {
                if(ISDBG)
                    fprintf(stderr, "Erreur de décodage d'un pacquet !\n");
                pkt_del(pkt);
                continue;
            }

            timestamp = pkt_get_timestamp(pkt);

            ret = treatPkt(buffer, &startBuffer, &curSeqNum, pkt, fd, &disp);


            if(ret >= 0)//TODO: move this where the packet hasn't been free yet
                sendACK(sfd, curSeqNum, timestamp, disp);

        }

    }while(ret != 3);//mean an EOF

    //sending one more ack to be sure that the sender get them
    sendACK(sfd, curSeqNum, timestamp, disp);

    free(buffer);

    return PKT_OK;
}

int treatPkt(pkt_t ** buffer, uint8_t *startBuf, uint8_t *curSeqNum, pkt_t * pkt, int fd, int * disp){

    printf("SeqNum: %d\t WSeqNum: %d\tDisp: %d\n", pkt_get_seqnum(pkt), *curSeqNum, *disp);
    fflush(stdout);

    uint8_t seqNum = pkt_get_seqnum(pkt);
    if(seqNum - (*curSeqNum) < 0)
        return 1;//a packet who's late and duplicate

    if(seqNum - (*curSeqNum) > BUFSIZE-1)
        return -2;//a packet from the future

    if(buffer[((*startBuf) + (seqNum - (*curSeqNum))) % BUFSIZE] == NULL) {
        buffer[((*startBuf) + (seqNum - (*curSeqNum))) % BUFSIZE] = pkt;
        (*disp) --;
    }
    else
        return 2;

    if(buffer[*startBuf] == NULL)//the current first packet is missing, skipping
        return 0;

    //at this point, there is some data to extract from the buffer (consecutive frames, at leat one)
    int i, size = 0;
    for(i = 0; i < BUFSIZE && (buffer[(*startBuf+i)%BUFSIZE]) != NULL; i++)
            size++;//counting the number of frame to output

    (*disp)+= size;
    int eof = 0;
    for(i = 0; i < size; i++) {//checking if the packet is an eof
        if(pkt_get_length(buffer[((*startBuf)+i)%BUFSIZE]) == 0 )
            eof = 1;

        //outputting the packets
        ssize_t written = write(fd, pkt_get_payload(buffer[(*startBuf + i) % BUFSIZE]), pkt_get_length(buffer[(*startBuf + i) % BUFSIZE]));
        if(written != pkt_get_length(buffer[(*startBuf + i) % BUFSIZE]))
            fprintf(stderr, "Attention ! Potentielle erreur d'écriture !\n");

        //freeing the packets
        pkt_del(buffer[(*startBuf + i) % 32]);

        //setting the buffer to NULL
        buffer[(*startBuf + i) % 32] = NULL;
    }
    //moving the current entry point of the buffer
    *startBuf = (uint8_t)((*startBuf+size) % BUFSIZE);

    //moving the current seqnum
    *curSeqNum = (uint8_t)((*curSeqNum+size) % 256);

    //returning
    return eof ? 3 : 1;
}

int sendACK(int sfd, uint8_t curNumSeq, uint32_t timestamp, int empty){
    fprintf(stderr ,"timestamp: %"PRIu32"\n", timestamp);
    //creating the packet
    pkt_t *pkt = pkt_new();
    pkt_set_length(pkt, 0);
    pkt_set_type(pkt, PTYPE_ACK);
    pkt_set_seqnum(pkt, (const uint8_t) (curNumSeq%256));
    pkt_set_window(pkt, (const uint8_t) (empty == 0 ? 0 : empty - 1));
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
