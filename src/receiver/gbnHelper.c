#include <stdio.h>
#include <stdlib.h>

#include "gbnHelper.h"

#define BUFSIZE 32

/*
 * fd : file descriptor where we're gonna write the output
 * sfd : socket file descritor
 * */
pkt_status_code selective_repeat(int fd, int sfd){
    pkt_t * buffer[BUFSIZE];
    uint8_t startWin = 0;//max size - 31
    //the seqnum is on 2^(n-1)



    return PKT_OK;
}

int treatPkt(pkt_t ** buffer, uint8_t startWin, char * data, const size_t len, int fd){
    //create the packet
    pkt_t *pkt = pkt_new();
    pkt_status_code status = pkt_decode(data, len, pkt);

    //discard it if he's broken
    if(status != PKT_OK)
        return -1;

    //try to insert it into the buffer
    uint8_t seqnum = pkt_get_seqnum(pkt);
    int i;
    for(i = 0; i < BUFSIZE; i++){
        if()
    }

    //send an ack for this packet with , in data, the "unrelied" data in buffer

    //check if there is some data to extract from the buffer (consecutive frames)
        //if there is data, send it on the file descriptor

    //advance the value wh'os tracking the current top of complete on the windows


    return 0;
}
