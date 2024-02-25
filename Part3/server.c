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
#include <sys/stat.h>
#include <stdbool.h>

#define MAXBUFLEN 100
#define MAX_ARRAY_SIZE 5000

// define packet, for a file that is larger than 1000 bytes, 
// needs to be fragmented and use multiple packets
struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

struct packet receivedPackets={0};

//function to recieve and read from UDP message queue and populate receivedPackets[]
void receivePackets(int sockfd, struct sockaddr_storage client_addr, socklen_t addr_len) {
    char recvBuffer[2000] = {0};
    
    // Keep receiving packets and build the file until it is the last one
    size_t receivedBytes = 0;
    if ((receivedBytes = recvfrom(sockfd, recvBuffer, 2000 , 0, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
        perror("recvfrom for packets");
        exit(1);
    }

    int itemMatched = 0;
    char filenameBuffer[1024];

    //after we take the message from UDP buffer to our own, we scan from the receive buffer and see if the format is correct
    // populate the first few fields within the struct in the global receivePackets[] buffer
    if((itemMatched = sscanf(recvBuffer, "%u:%u:%u:%[^:]:", &(receivedPackets.total_frag), &(receivedPackets.frag_no),
                                                        &(receivedPackets.size), filenameBuffer)) < 0) {
        perror("unable to scan from string received.\n");
        exit(1);
    }

    //copy the file name field and copy the data portion
    int offset = snprintf(NULL, 0, "%u:%u:%u:%s:", receivedPackets.total_frag, receivedPackets.frag_no, 
                                                receivedPackets.size, filenameBuffer);
    receivedPackets.filename = strdup(filenameBuffer);
    memcpy(receivedPackets.filedata, recvBuffer+offset, receivedPackets.size);

    //debug
    printf("server: received packet number %d out of %d.\n",receivedPackets.frag_no, receivedPackets.total_frag);

}

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
    char buf[MAXBUFLEN] = {0};
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    char* port, *response;

    // if(argc != 2){
    //     fprintf(stderr,"Wrong number of input: server <UDP listen port>\n");
    //     exit(1);
    // }
    port = "55000"; //populate port from command line argument

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

    // //debug
    // printf("%s\n", buf);
    // int result = strcmp(buf, "ftp");
    // printf("%d\n", result);
    // return 0;

    printf("server: got packet from %s\n",
        inet_ntop(client_addr.ss_family,
            get_in_addr((struct sockaddr *)&client_addr),
            s, sizeof s));
    printf("server: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("server: packet contains message \"%s\"\n", buf);


    //check if the content is "ftp", reply with result
    if(strcmp(buf, "ftp") == 0){ 
        response = "yes";
    }else{ 
        response = "no";
    }

    ssize_t responseByte;
    if( (responseByte = sendto(sockfd, response, sizeof(response), 0, (struct sockaddr *)&client_addr, addr_len)) == -1 ){
        perror("response sendto");
        exit(1);
    }

    //after server allows to be connected, we wait again to recieve file from client

    // Create the directory if it doesn't exist
    const char* directory = "./received_files";
    if (mkdir(directory, 0777) == -1 && errno != EEXIST) {
        perror("mkdir");
        exit(EXIT_FAILURE);
    }

    while(1){
        
        //idea: the server always wait for packets to receive. it first check if there is a receive dir, if not it makes one
        //      then, it calls receivePackets which receives the VERY FIRST packet, this is because we need to do fopen for new file
        //      then, it goes into the while loop to receive the rest of the packet and send ACK with each one.


        //make a new file for the first packet received
        receivePackets(sockfd, client_addr, addr_len);
        // Construct the file path
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, receivedPackets.filename);
        // Open the file in binary append mode
        FILE *file = fopen(filepath, "w");
        if(file == NULL){
            perror("Cannot create new file.\n");
            exit(1);
        }

        //part3
        bool dropped = 0;

        while (receivedPackets.frag_no <= receivedPackets.total_frag) {
            if( (receivedPackets.frag_no == 5) && (dropped == 0) ){
                dropped = 1;
            }else{
                // move data to file created
                fwrite(receivedPackets.filedata, 1, receivedPackets.size, file);
                // send ack 
                char ack_buffer[] = "ACK";
                if ((responseByte = sendto(sockfd, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&client_addr, addr_len)) == -1) {
                    perror("response sendto");
                    exit(1);
                }

                // If the last packet is received, break out of the loop
                if (receivedPackets.frag_no == receivedPackets.total_frag) {
                    break;
                }
            }
            // Receive the next packet
            receivePackets(sockfd, client_addr, addr_len);
        }

        fclose(file);
        printf("server: finished writing to file.\n");

    }

    close(sockfd);

    return 0;
}