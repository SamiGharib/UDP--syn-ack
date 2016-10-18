#include <stdio.h>
#include <stdlib.h>

#include "gbnHelper.h"

/*
 * fd : file descriptor where we're gonna write the output
 * sfd : socket file descritor
 * */
pkt_status_code selective_repeat(int fd, int sfd){//TODO

    return PKT_OK;
}

/*
 * the stuffing is a stream cypher
 * DLE : 0x10
 * STX : 0x02
 * ETX : 0x03
 * */

int charStuffing(unsigned char ** db, int len){//TODO
    if(len < 0)return -1;
    //first pass computer the size of the ouput buffer by parsing all the data
    int size = 4+len, i, decal = 0;
    for(i = 0; i < len ; i++)
        if((*db)[i] == 0x10)//counting the number of DEL char into the frame
            size++;

    //on the seccond pass we're coping all the data from the first to the seccond buffer with the char stuffing included
    unsigned char * new = (unsigned char*)malloc(sizeof(unsigned char)*size);
    new[0] = 0x10;//DLE
    new[1] = 0x02;//STX
    for(i = 0; i < len ; i++) {
        if ((*db)[i] == 0x10) {
            new[2+i+decal] = 0x10;//DLE
            decal++;
            new[2+i+decal] = 0x10;//DLE
        }
        else
            new[2+i+decal] = (*db)[i];
    }
    new[size-2] = 0x10;//DLE
    new[size-1] = 0x03;//ETX

    //we're calling free on the old buffer and set the pointer to the new one
    free(*db);
    *db = new;
    return size;
}

int charUnstuffing(unsigned char ** db, int len){
    //checking the first and last marker
    if((*db)[0] != 0x10 || (*db)[1] != 0x02 || (*db)[len-2] != 0x10 || (*db)[len-1] != 0x03)
        return -1;

    //first pass to get the future size of the new buffer
    int i, size = len -4, filler;
    for(i = 2; i < len -2; i++)
        if((*db)[i] == 0x10) {//checking for an escap mark DEL
            size--;
            i++;
            if((*db)[i] != 0x10)//unexpected character, the frame is currupted, aborting
                return -1;
        }

    //the data are valid, seccond pass to unstuff the data
        //creating the new buffer
    unsigned char *new = (unsigned char *)malloc(sizeof(unsigned char)*size);

    for(i = 2, filler = 0; i < len -2; i++, filler++){
        if((*db)[i] == 0x10)
            i++;
        new[filler] = (*db)[i];
    }

    //repointing all the buffers
    free(*db);
    *db = new;
    return -1;
}