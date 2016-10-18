//
// Created by pierre on 16/10/16.
//

#include "../shared/packet_interface.h"

#ifndef RECEIVER_BGNHELPER_H
#define RECEIVER_BGNHELPER_H

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
 * @param startWin : the current first emplacement of the sliding windows
 * @param data : the char extracted from the file who's ontaining all the data
 * @param len : sizeof the data in byte
 * @param fd : the file descriptor into witch the data are outputed
 * @return : the value returned is the status of the application :
 *      -1 : the packet is broken
 *      0 : everything is ok packet has been incorporated
 *      1 : the packet is ok, but already into the buffer
 * */
int treatPkt(pkt_t **buffer, uint8_t startWin, char * data, size_t len, int fd);

#endif
