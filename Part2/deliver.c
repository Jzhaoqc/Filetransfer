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
#define MAX_ARRAY_SIZE 1000

// define packet, for a file that is larger than 1000 bytes, 
// needs to be fragmented and use multiple packets
struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

//global array, used to store packets of files to send to server
struct packet packets[MAX_ARRAY_SIZE];


size_t findSize(FILE *fptr){

    if(fptr == NULL){
        perror("Can not open file");
        exit(1);
    }
    if(fseek(fptr,0,SEEK_END)<0){
        fclose(fptr);
        perror("fseek failure");
        exit(1);
    }
     
    return ftell(fptr);
}

int findPacketNumber(size_t size){
    if((size%1000)!=0){
        return (size/1000)+1;
    }else{
        return size/1000;
    }
}

//talker
int main(int argc, char *argv[]){
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    char* ip;
    char *port, *hostname;
    char filePath[100] = "./send/"; 
    char fileName[100];
    char cmd[100]={0};

    if (argc != 3) {
        fprintf(stderr,"Wrong number of input: deliver <server address> <server port number>\n");
        exit(1);
    }

    //extracting hostname from command line input for DNS lookup
    hostname = argv[1];
    port = argv[2];
    printf("client: input port %s\n", port);
    printf("client: input hostname %s\n", hostname);

    //init addrinfo struct, populate target spec for reference in getaddrinfo()
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    //result will populate servinfo
    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket ERROR");
            continue;
        }

        //ip address sanity check
        struct sockaddr_in *ip_access = (struct sockaddr_in *) p->ai_addr;
        ip = inet_ntoa(ip_access->sin_addr);
        printf("client: IP from DNS lookup is %s\n", ip);
    
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "deliver: failed to create socket\n");
        return 2;
    }
    freeaddrinfo(servinfo);
    
    //After this point, we have gotten the server's address and all relative info
    //can use the structure instance p in sendto() and recvfrom() functions for data transmission


    //get path to file, check existence
    printf("client: File to transfer: ftp <file name>\n");
    scanf("%s %s", cmd, fileName);
    if(strcmp(cmd,"ftp") != 0){     //check if the first command from user is "ftp" print error if not
        perror("Wrong command: not ftp\n");
        exit (1);
    }
    strcat(filePath,fileName);
    printf("client: trying to access: %s\n", filePath);
    if(access(filePath, F_OK) != 0){
        perror("File doesn't exist\n");
        exit (1);
    }

    //sending message "ftp" to server to request for file transfer
    char msg[4] = "ftp";
    if((numbytes = sendto(sockfd, msg, sizeof(msg), 0, p->ai_addr, p->ai_addrlen)) == -1){
        perror("failed to send\n");
        exit (1);
    }
    //sanity check
    printf("client: sent %d bytes to %s\n", numbytes, argv[1]);

    //block until server responds with either "yes" or "no"
    char resposeBuf[100];
    if ((numbytes = recvfrom(sockfd, resposeBuf, sizeof(resposeBuf), 0, p->ai_addr, &(p->ai_addrlen))) == -1) { //NOTE: the p->ai_addr field should have an empty struct
        perror("recvfrom");                                                                                     //which will then gets populated, reused p since they should be the same
        exit(1);
    }

    if(strcmp(resposeBuf, "yes") == 0){
        printf("client: A file transfer can start.\n");
    }else{
        perror("server refused transfer\n");
        exit (1);
    }

    // sanity check, can still send
    //sendto(sockfd, "can still send\n", sizeof("can still send\n"), 0, p->ai_addr, p->ai_addrlen);

    //after established connection with the server, we now get the file, 
    //transform it to packet(s) if data is larger than 1000bytes, and send to server
    /*
    todo:
        open file
        transform into socket struct
        sendto server
    */

    FILE *fptr;
    fptr = fopen(filePath, "rb");   //rb stand for read binary
    size_t fileSize = findSize(fptr);
    int totalPackets = findPacketNumber(fileSize);

    for(int i=1; i<totalPackets; i++){
        struct packet newPacket = {0};
        newPacket.total_frag = totalPackets;
        newPacket.frag_no = i;
        //finish filling in the rest of the fields

        packets[i-1]=
    }
    


    close(sockfd);

    return 0;

}
