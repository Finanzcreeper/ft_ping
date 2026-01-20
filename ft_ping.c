#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/ip_icmp.h>

// Define the Packet Constants
#define PING_PKT_S 64       // ping packet size
#define PING_SLEEP_RATE 1000000  // ping sleep rate (in microseconds)
#define RECV_TIMEOUT 1      // timeout for receiving packets (in seconds)

void display_help(){
    printf("HELP TEXT HERE!\n");
    return;
}

unsigned short checksum(void *b, int len) {
    unsigned short* buffer = b;
    unsigned int sum = 0;
    unsigned short result;

    while(len > 1) {
        sum += *buffer++;
        len -= 2;
    }

    if (len == 1) {
        sum += *(unsigned char *)buffer;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

struct ping_pkt {
    struct icmphdr hdr;
    char msg[PING_PKT_S - sizeof(struct icmphdr)];
};

int main(int argc, char* argv[]) {
    char* option = "";
    char* adress = "";
    int verbose = 0;

    switch (argc) {
        case 2:
            if (argv[1][0] == '-'){
                option = argv[1];
            } else {
                adress = argv[1];
            }
            break;
        case 3:
            if (argv[1][0] == '-'){
                option = argv[1];
            }
            adress = argv[2];
            break;
        default:
            printf("Usage: ping [Options] <ADDRESS> \n");
            return(0);
            break;
    }
    if (strcmp(option, "-v") == 0) {
        verbose = 1;
    } else if (strcmp(option, "-V") == 0){
        printf("ft_ping Version 0.69\n");
        return(0);
    } else if (option != "") {
        display_help();
        return(0);
    }
    if (adress == NULL){
        printf("ping: usage error: Destination address required\n");
        return(0);
    }

    struct addrinfo* res;
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int err = getaddrinfo(adress, NULL, &hints, &res);
    if (err < 0) {
        const char* errmsg = gai_strerror(err);
        printf ("%s\n",errmsg);
        return(0);
    }
    int ping_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (ping_socket < 0) {
        printf("\nSocket file descriptor not received!\n");
        free(res);
        return (0);
    } else {
        printf("\nSocket file descriptor %d received\n", ping_socket);
    }

    int message_amount = 0;
    struct ping_pkt pckt;
    bzero(&pckt, sizeof(pckt));
    pckt.hdr.type = ICMP_ECHO;
    pckt.hdr.un.echo.id = getpid();
    int c = 0;
    while ( c < sizeof(pckt.msg) - 1) {
        pckt.msg[c] = c + '0';
        c++;
    }
    pckt.msg[c] = 0;
    pckt.hdr.un.echo.sequence = message_amount++;
    pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));

    err = sendto(ping_socket, pckt.msg, sizeof(pckt.msg), 0, res->ai_addr, res->ai_addrlen);
    if (err == -1) {
        printf("ERROR: %s\n", strerror(errno));
        free(res);
        close(ping_socket);
        return(0);
    }

    free(res);
    close(ping_socket);
    printf("hello world!\n");
}