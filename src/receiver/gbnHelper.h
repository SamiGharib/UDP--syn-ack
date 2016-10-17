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
 * The goal of this function is to make the caracter stuffing on a data buffer.
 * @param db : it's the data buffer, the one who need bit stuffing.
 * The pointer is gonna be modified for the new buffer (witch is gonna be bigger - in
 * theory - if I don't fuck it up !)! The old buffer is gonna be free.
 * @param len : the size fo the input buffer (in byte)
 * @return the new size (in byte) of the data buffer db after the character stuffing or -1 in case of error
 * */
int charStuffing(unsigned char ** db, int len);

/**
 * The goal of this function is to remove the caracter stuffing on a data
 * buffer.
 * @param db : it's the data buffer who's gonna contain the data to unstuff, the pointer is gonna
 * be move at the end of the function to point to the new buffer, the one containing the unstuffed
 * data.
 * @param len : it's the size of the inputed buffer (in byte)
 * @return the new size (in byte) of the data buffer after the caracter unstuffing or -1 in case of
 * error / corupted buffer
 * */
int charUnstuffing(unsigned char ** db, int len);

#endif //RECEIVER_BGNHELPER_H
