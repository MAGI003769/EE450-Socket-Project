/**
**
** client.c
** Author: Ruihao Wang
** USC-ID#: 9867439484
** Code is modified and expanded base on stream client source in beej
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <ctype.h>

#define PORT "34484"    // the TCP port of AWS which client connects to
#define IPADDRESS "localhost" // source and destination address
#define MAXBUFLEN 4000   // the maximum number of bytes


// funciton to get socket address from a sockaddr struct
void *get_in_addr(struct sockaddr *sa) {
    if(sa->sa_family == AF_INET6) {
        // IPv6
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }
    // IPv4
    return &(((struct sockaddr_in*)sa)->sin_addr);
}

// parse the calculated results
void parse_result(char *args[], char *message) {
    char *p = strtok (message, "\n");
    int i = 0;
    while (p != NULL)
    {
        args[i++] = p;
        p = strtok (NULL, "\n");
    }
}

// format printing
void print_result(char *src, char *target, char *args[]) {
    printf("The client has received results from AWS: \n");
    printf("-------------------------------------------------------------------------------\n");
    printf("Source\tDestination\tMin Length\tTt\tTp\tDelay\n");
    printf("-------------------------------------------------------------------------------\n");
    printf("%s\t\t%s\t%s\t%s\t%s\t%s\n", src, target, args[0], args[1], args[2], args[3]);
    printf("-------------------------------------------------------------------------------\n");
    printf("%s\n", args[4]);
}

int main(int argc, char *argv[]) {

    int sockfd, numbytes;  
    char buf[MAXBUFLEN];
    char *results[5];
    char args_str[100];
    char check[10];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    // check whether aguments enough
    if(argc != 5) {
        fprintf(stderr, "usage Error: client function needs 4 arguments <map_id> <node_1> <node_2> <file_size>\n");
        exit(1);
    }

    // check valid arguments or not
    // if(isalpha(argv[0]) != 0) {
    //     fprintf(stderr, "usage Error: MAP ID should be alphabetic.\n");
    //     return 1;
    // }

    // load up address structs with getaddrinfo()
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(IPADDRESS, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("The client is up and running.\n");

    freeaddrinfo(servinfo); // all done with this structure

    // merge args into single string to one-time send
    sprintf(args_str, "%s %s %s %s", argv[1], argv[2], argv[3], argv[4]);

    if ((numbytes = send(sockfd, args_str, strlen(args_str), 0)) == -1) {
        perror("send");
        exit(1);
    }
    fflush(stdout);
    //printf("%s has been sent\n", args_str);

    // for(int i = 1; i <= 4; i++) {
    //     if ((numbytes = send(sockfd, argv[i], strlen(argv[i]), 0)) == -1) {
    //         perror("send");
    //         exit(1);
    //     }
    //     fflush(stdout);
    //     printf("%s has been sent\n", argv[i]);
    // }

    printf("The client has sent query to AWS using TCP: start vertex %s; destination vertex %s; map %s; file size %s\n", argv[2], argv[3], argv[1], argv[4]);

    if((numbytes = recv(sockfd, buf, MAXBUFLEN-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    // fflush(stdout);
    // printf("%d \n", numbytes);
    buf[numbytes] = '\0';
    //strncpy(check, buf, 2);
    //printf("debug: check is %s\n", check);
    if(strstr(buf, "No") != NULL) {
        printf("%s\n", buf);
    } else {
        parse_result(results, buf);
        print_result(argv[2], argv[3], results);
    }

    close(sockfd);

    return 0;
}
