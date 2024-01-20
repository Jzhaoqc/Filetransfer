#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//#define SERVERPORT "55000" // the port users will be connecting to

//The networking libraries can be daunting at first, but the 
//essential functions to focus on are: socket, bind, sendto, and recvfrom.


//The "Datagram Sockets" section under "Client-Server Background" is the most
//relevant sample code for this lab since you're required to use
//UDP. 

//talker
int main(int argc, char *argv[]){
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    char* ip;
    char *port, *hostname;

    if (argc != 3) {
        fprintf(stderr,"Wrong number of input: deliver <server address> <server port number>\n");
        exit(1);
    }

    //extracting hostname for DNS lookup
    hostname = argv[1];
    port = argv[2];
    printf("port: %s\n", port);
    printf("hostname: %s\n", hostname);


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("deliver: socket");
            continue;
        }
        //ip address sanity check
        struct sockaddr_in *ip_access = (struct sockaddr_in *) p->ai_addr;
        ip = inet_ntoa(ip_access->sin_addr);
        printf("IP from DNS lookup: %s\n", ip);
        return 0;
    
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "deliver: failed to create socket\n");
        return 2;
    }


    if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
            p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("deliver: sent %d bytes to %s\n", numbytes, argv[1]);
    close(sockfd);

    return 0;

}
