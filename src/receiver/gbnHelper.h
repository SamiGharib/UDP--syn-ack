//
// Created by pierre on 16/10/16.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <netinet/in.h>
#include <malloc.h>
#include <inttypes.h>


#include "../shared/packet_interface.h"

#ifndef RECEIVER_BGNHELPER_H
#define RECEIVER_BGNHELPER_H

#define MAXWIN 31

/**
 * This function is called when a client connect, but no data from the connexion has already been
 * consumed. The  main goal of this function is to apply the go-back-n algorythm until it encounter
 * an EOF and it has all the file.
 * @param fd : it's the file descriptor into with we are gonna write what we're recomposing
 * @param sfd : it's the socket file descirptor into witch we are gonna receive the data from the sender.
 * @return pkt_status_code : it return PKT_OK if everting whent correctly or the apropriate error code
 * if a problem has occured
 * */
pkt_status_code selective_repeat(int fd, int sfd);

/**
 * The goal of this function is to manage the packet that has juste been receive by the network
 * @param buffer : the buffer containing all the packet who've already bben received, if the is some place,
 * it's gonna be NULL
 * @param startBuf : the current first emplacement of the sliding windows
 * @param curSeqNum :
 * @param data : the char extracted from the file who's ontaining all the data
 * @param pkt : the packet who've been just received and who's gonna be threated
 * @param fd : the file descriptor into witch the data are outputed
 * @return : the value returned is the status of the application :
 *      < 0 : don't send an ack, there is a problem with the packet, need another
 *      >= 0 : send ack the packet is or already have been treated
 *
 *  CODES:
 *      -2 : the packet is too far after the windows and can't be incorporated yet - when the sender don't care about the windows
 *      -1 : the packet is broken
 *      0 : everything is ok packet has been incorporated
 *      1 : the packet is out of the windows and have already been render to the user
 *      2 : the packet have already been received and is currently into the buffer
 *      3 : EOF
 * */
int treatPkt(pkt_t **buffer, uint8_t *startBuf, uint8_t *curSeqNum, pkt_t * pkt, int fd);


/**
 * @param sfd : the socket file desciptor where the ack is gonna be send
 * @param curNumSeq : the sequence number for the next waited paquet
 * @return : 0 on sucess and -1 if the write hasn't suceed
 * */
int sendACK(int sfd, uint8_t curNumSeq, uint32_t timestamp);

#endif
