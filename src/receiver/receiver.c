#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h> //for getopt

#include "../shared/packet_interface.c"
#include "../shared/real_address.c"
#include "../shared/wait_for_client.c"
#include "../shared/create_socket.h"
#include "gbnHelper.h"
#include "receiver.h"

int main(int argc, char ** argv){
    int port, opt, file = -1;
    //TODO modify the way we handle the extenal files to have a name and a boolean instead of juste a name (much simpler)
    char *host;

    //receiver [-f X] <host> <port>
    while((opt = getopt(argc, argv, "f:")) != -1){
        switch (opt) {
            case 'f':
                file = fileno(fopen(optarg, "w"));
                break;
            default:
                fprintf(stderr, "Usage: receiver [-f X] <host> <port>\n");
                return EXIT_FAILURE;

        }
    }

    if (optind+1 >= argc) {
        host = "::1";
        port = 12345;
        fprintf(stderr, "Usage: receiver [-f X] <host> <port>\n");
        fprintf(stderr, "localhost and port 12345 are gonna be used.\n");
    }
    else {
        host = argv[optind];
        port = atoi(argv[optind +1]);
        if(port < 0 || port > 65536){//a bit of defensive prog on the port
            fprintf(stderr, "The given port isn't valid! Default used : \"12345\"\n");
            port = 12345;
        }
    }

    if(file == -1) {
        fprintf(stderr, "Output set to stdout\n");
        file = fileno(stdout);
    }

    //binding to the local socett and port
    struct sockaddr_in6 addr;
    const char *err = real_address(host, &addr);
    if (err) {
        fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
        return EXIT_FAILURE;
    }

    //TODO: maybe transform that into a function
    int sfd = create_socket(&addr, port, NULL, -1);
    if(sfd > 0 && wait_for_client(sfd) < 0){
        fprintf(stderr, "Could not connect the socket after the first message.\n");
        close(sfd);
        return EXIT_FAILURE;
    }
    if(sfd < 0){
        fprintf(stderr, "Failed to create the socket!\n");
        return EXIT_FAILURE;
    }


    //receive the binary file here
    pkt_status_code status = selective_repeat(file, sfd);
    if(status != PKT_OK)
        return EXIT_FAILURE;

    if(file != fileno(stdout))
        close(file);
    close(sfd);

    return EXIT_SUCCESS;
}