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

#define MAXBUFLEN 100

void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    perror("not an IPv4 connection, got IPv6\n");
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//listener
int main(int argc, char *argv[]){
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage client_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    char* port, *response;

    if(argc != 2){
        fprintf(stderr,"Wrong number of input: server <UDP listen port>\n");
        exit(1);
    }
    port = argv[1]; //populate port from command line argument
    // printf("%s\n", port);
    // return 0;

    //init and populate the structure that specifies the criteria for selecting socket address 
    //returned in the list pointed by serviceinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    //Idea: find available address, bind to socket so that the client can reach this server
    //getaddrinfo allocates and init a linked list of addrinfo struct, for each network matches node and service criteria set in hints
    //might have more than 1 list node if network host is multihomed etc. all linked by ai_next field

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {   //should return 0 unless error
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("server: socket");
            continue;
        }
        //we do not want the server address to change, therefore binding to it
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    if (p == NULL) {    //check if there is a usable address found from earlier
        fprintf(stderr, "server: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("server: listening to port %s...\n", port);


    //after this point, the address is binded and server is listening and waiting to recieve
    //after recieving the structure client_addr would get populated and can be used to respond


    //read from UDP message queue
    addr_len = sizeof client_addr;
    //read the first msg in queue and populate client_addr with source address
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf("server: got packet from %s\n",
        inet_ntop(client_addr.ss_family,
            get_in_addr((struct sockaddr *)&client_addr),
            s, sizeof s));
    printf("server: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("server: packet contains message \"%s\"\n", buf);

    //check if the content is "ftp", reply with result
    if(strcmp(buf, "ftp") == 0){ response = "yes";}
    else{ response = "no";}
    int responseByte;
    if( (responseByte = sendto(sockfd, response, sizeof(response), 0, (struct sockaddr *)&client_addr, addr_len)) == -1 ){
        perror("response sendto");
        exit(1);
    }

    // sanity check, can still recieve
    // recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&client_addr, &addr_len);
    // printf("%s", buf);

    close(sockfd);

    return 0;
}
