/**
**
** serverB.c
** Author: Ruihao Wang
** USC-ID#: 9867439484
**
** Code is modified and expanded base on stream server source in beej
** Specificly, backend server implementation refers to listener.c
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
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#define IPADDRESS "127.0.0.1"  // local IP address
#define MYPORT "31484"	// the port used for UDP connection with AWS
#define MAPDIR "./maps/map2.txt" // file directory to get map information
#define MAXBUFLEN 4000
#define MAPNOTFOUND "Map not found"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// split received message
void split_args2(char *args[], char *message) {
    char *p = strtok (message, " ");
    int i = 0;
    while (p != NULL)
    {
        args[i++] = p;
        p = strtok (NULL, " ");
    }
}

// serach map base on map_id
void search_map(char *map_id, char *map_data) {
    // open corresponding txt file
    FILE *file = fopen(MAPDIR, "r");
    if(file == NULL) {
        printf("File %s not found", MAPDIR);
        return;
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    read = getline(&line, &len, file);
    strcpy(map_data, line);
    //strcat(data, "\n");
    
    if(map_id[0] != line[0]) {
        while ((read = getline(&line, &len, file)) != -1) {
            //printf("Retrieved line of length %zu:\n", read);
            if(map_id[0] == line[0]) {
                break;
            }
        }
    }

    if(map_id[0] != line[0]) {
        strcpy(map_data, MAPNOTFOUND);
        return;
    } else {
        strcpy(map_data, line);
        while ((read = getline(&line, &len, file)) != -1) {
            if(isalpha(line[0]) != 0) {
                break;
            }
            strcat(map_data, line);
        }
    }
}

int main(void) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    char buf[MAXBUFLEN];
    char data[MAXBUFLEN];
    char *args[3];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    // get information of the backend server itself with MYPORT
    if((rv = getaddrinfo(IPADDRESS, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    if(p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    addr_len = sizeof their_addr;
    
    printf("The Server B is up and running using UDP on port %s\n", MYPORT);
    fflush(stdout);

    while(1) {
        if((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';

        // split messages into map_id, source_vtx and target_vtx 
        split_args2(args, buf);

        // read file and search the map_id
        //memset(&buf, 0, sizeof buf);
        search_map(args[0], data);

        printf("The Server B has received input for finding graph of map %s\n", args[0]);

        numbytes = sendto(sockfd, data, strlen(data), 0,
            (struct sockaddr *)&their_addr, addr_len);
        
        if(numbytes == -1) {
            perror("listener: sendto");
            exit(1);
        }

        if(strcmp(data, MAPNOTFOUND) == 0) {
            printf("The Server B does not have the required graph id %s\n", args[0]);
            printf("The Server B has sent \"Graph not Found\" to AWS\n");
        } else {
            printf("The Server B has sent Graph to AWS\n");
        }
        
        //close(sockfd);
    }

    return 0;

}
