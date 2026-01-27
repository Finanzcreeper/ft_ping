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
#include <signal.h>
#include <sys/time.h>

// Define the Packet Constants
#define PING_PKT_S 64       // ping packet size
#define PING_SLEEP_RATE 1000000  // ping sleep rate (in microseconds)
#define RECV_TIMEOUT 1      // timeout for receiving packets (in seconds)

int running = 1;

struct ping_pkt {
    struct icmphdr hdr;
    char msg[PING_PKT_S - sizeof(struct icmphdr)];
};

void signalHandler(int sig) {
    running = 0;
}

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
int setup(char* adress, int* ping_socket, struct addrinfo** res, char* host){
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;

    int err = getaddrinfo(adress, NULL, &hints, res);
    if (err != 0) {
        const char* errmsg = gai_strerror(err);
        printf ("%s\n",errmsg);
        return(-1);
    }

    err = getnameinfo((*res)->ai_addr, (*res)->ai_addrlen, host, NI_MAXHOST, NULL, 0, NI_NAMEREQD);
    if(err != 0) {
        printf("Could not resolve reverse lookup of hostname\n");
        const char* errmsg = gai_strerror(err);
        printf ("%s\n",errmsg);
        return (-1);
    }

    *ping_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (ping_socket < 0) {
        printf("Socket file descriptor not received!\n");
        free(res);
        return (-1);
    }
}

struct ping_pkt create_packet(int* message_nr) {
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
    pckt.hdr.un.echo.sequence = (*message_nr)++;
    pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
    return (pckt);
}

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
    int ping_socket = 0;
    struct addrinfo* res;
    char host[NI_MAXHOST];
    if(setup(adress, &ping_socket, &res, host) == -1) {
        return(0);
    }
    int sent_message_amount = 0;
    int message_sent = 1;
    int recieved = 0;
    char return_buffer[1000];
    struct ping_pkt pckt = create_packet(&sent_message_amount);
    void* addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    
    char ipstr[16];
    bzero (ipstr, 16);
    if(inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr)) == NULL) {
        printf("ERROR Adrres could not be translated to human readable form\n");
    }
    printf("PING %s (%s)(%s) %ld bytes of data. \n",adress, ipstr, host, sizeof(pckt));

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(ping_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int return_message_amount = 0;
    sent_message_amount = 0;
    signal(SIGINT, signalHandler);

    struct timeval send_time;
    struct timeval recieve_time;
    while(running == 1) {
        struct ping_pkt pckt = create_packet(&sent_message_amount);
        message_sent = 1;
        int sent_amount = sendto(ping_socket, &pckt, sizeof(pckt), 0, res->ai_addr, res->ai_addrlen);
        if (sent_amount == -1) {
            sent_message_amount -= 1;
            printf("ping: sending packet: %s\n", strerror(errno));
            message_sent = 0;
        }
        int err = gettimeofday(&send_time, NULL);
        if (err != 0) {
            printf("ping: cannot get sending time: %s\n", strerror(errno));
        }
        if (message_sent == 1) {
            struct sockaddr_in recieve_address;
            int r_addr_len = sizeof(recieve_address);
            bzero(return_buffer,sizeof(return_buffer));
            recieved = recvfrom(ping_socket, return_buffer, sizeof(return_buffer), 0, (struct sockaddr*)&recieve_address, &r_addr_len);
            if(recieved <= 0) {
                printf("Recieve ERROR: %s\n", strerror(errno));
                recieved = 0;
            }
            int err = gettimeofday(&recieve_time, NULL);
            if (err != 0) {
                printf("ping: cannot get recieve time: %s\n", strerror(errno));
            }

            long sec_diff = recieve_time.tv_sec - send_time.tv_sec;
            long usec_diff = recieve_time.tv_usec - send_time.tv_usec;
            int ms_pre_comma = sec_diff * 1000 + usec_diff / 1000;
            int ms_after_comma = sec_diff * 1000 + usec_diff % 1000;
            printf("%d bytes from %s: icmp_seq:=%d, time=%d,%d ms NEW\n", recieved, host, sent_message_amount, ms_pre_comma, ms_after_comma);

            if (checksum(return_buffer, recieved) == 0) {
                ++return_message_amount;
            }
        }
        usleep(1000000);
    }

    float packetloss_percentage = (sent_message_amount - return_message_amount) / (double)sent_message_amount * 100.0;
    printf("--- %s ping statistics ---\n", adress);
    printf("%d packets transmitted, %d packtes recieved, %f%% packet loss\n", sent_message_amount, return_message_amount, packetloss_percentage);

    free(res);
    close(ping_socket);
}