/**
**
** aws.c
** Author: Ruihao Wang
** USC-ID#: 9867439484
** Code is modified and expanded base on stream server source in beej
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

#define PORTCLIENT "34484"     // the port for TCP with client

#define UDPPORT "33484" // the port use UDP

#define SERVERPORTA "30484"	   // the port users will be connecting to 
							   // Backend-Server (A)
#define SERVERPORTB "31484"	   // the port users will be connecting to 
							   // Backend-Server (B)
#define SERVERPORTC "32484"	   // the port users will be connecting to 
							   // Backend-Server (C)
#define IPADDRESS "127.0.0.1"  // local IP address

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAPNOTFOUND "Map not found"

#define MAXBUFLEN 4000

// from beej
void sigchld_handler(int s){
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    
    while(waitpid(-1, NULL, WNOHANG) > 0);
    
    errno = saved_errno;
}

// funciton to get socket address from a sockaddr struct, from beej
void *get_in_addr(struct sockaddr *sa) {
    if(sa->sa_family == AF_INET6) {
        // IPv6
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }
    // IPv4
    return &(((struct sockaddr_in*)sa)->sin_addr);
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

// setup TCP with client at port
int setupTCP(char* port) {
    int rv;
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sigaction sa;
    int yes = 1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(IPADDRESS, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            perror("setsockopt");
            exit(1);
        }
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if(p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if(listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    
    sa.sa_handler = sigchld_handler; // reap all dead processes sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1); 
    }

    return sockfd;
}

/*
** Following two functions are used to complete the UDP connection from AWS
** to a backend server. The implementation is based on talker.c in beej.
*/

// UDP socket bind with 33xxx port
// Just create UDP socket, no send and receive operations
int setupUDP2(char* port)
{
	int sockfd;
	int rv;
	struct addrinfo hints, *servinfo, *p;
	socklen_t addr_len;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(IPADDRESS, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	freeaddrinfo(servinfo); // done with servinfo
	return sockfd;
}

// use the same UDP socket to query based on specified port information
void udpQuery2(int sockfd, char *query, char *port, char *data) {
    //int server_sock;
    int numbytes;
    int rv;
    struct addrinfo hints, *servinfo, *p;
	socklen_t addr_len;
	memset(&hints, 0, sizeof hints);
    char recv_data[MAXBUFLEN]; // data received from backend server
    
    //sockfd = setupUDP(query, port);
    if ((rv = getaddrinfo(IPADDRESS, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}

    for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
        break;
    }

    if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return;
	}

    //printf("debug: find backend info...\n");
    
    // send map_id, source_vtx and target_vtx to server
	if ((numbytes = sendto(sockfd, query, strlen(query), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

    //printf("debug: aws has sent query...\n");

	if (strcmp(port, SERVERPORTA)==0) {
		printf("The AWS has sent map ID to server A using UDP over port %s\n", UDPPORT);
	} else if (strcmp(port, SERVERPORTB)==0) {
		printf("The AWS has sent map ID to server B using UDP over port %s\n", UDPPORT);
    } else {
        printf("The AWS has sent map, source ID, destination ID, propagation speed and transmission speed to server C using UDP over port %s\n", UDPPORT);
    }

    int recv_bytes;

    recv_bytes = recvfrom(sockfd, recv_data, sizeof recv_data, 0, NULL, NULL);
    if(recv_bytes == -1) {
        perror("recvfrom");
        exit(1);
    }
    recv_data[recv_bytes] = '\0';
    //printf("debug: AWS receives UDP: %s\n", recv_data);
    //printf("debug: AWS receives UDP data length: %d\n", recv_bytes);
    //printf("debug: AWS receives UDP bytes: %d\n", recv_bytes);
    //close(sockfd);
    strcpy(data, recv_data);
    //printf("debug: %s\n", data);
    //return return_recv_data;
}


/* 
 * Function: check_vtx
 * ----------------------------------
 *  function to check whether the source and target vertices 
 *  are in the request map or not
 *  
 *  source_vtx: string of source vertex
 *  target_vtx: string of target vertex
 *  data: string of map data from backend through UDP
 *
 *  return value int: 
 *   - 0 neither dound
 *   - 1 only source found
 *   - 10 only target found
 *   - 11 both found
 */
int check_vtx(char *source_vtx, char *target_vtx, char *data) {
    int source_flag = 0, target_flag = 0;
    char *end_str;
    char *token = strtok_r(data, "\n", &end_str);
    int i = 0;

    while (token != NULL) {
        char *end_token;
        //printf("debug: check line -- %s\n", token);
        if(i >= 3) {
            char *token2 = strtok_r(token, " ", &end_token);
            int j = 0;
            while (token2 != NULL && j < 2) {
                //printf("debug: check word -- %s\n", token2);
                if(strcmp(token2, source_vtx) == 0) {
                    source_flag = 1;
                } else if(strcmp(token2, target_vtx) == 0) {
                    target_flag = 1;
                }

                if(target_flag == 1 && source_flag == 1) {
                    return 11;
                }

                token2 = strtok_r(NULL, " ", &end_token);
                j++;
            }
        }
        token = strtok_r(NULL, "\n", &end_str);
        i++;
    }
    return (target_flag * 10 + source_flag);
}



int main(void) {
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	int udp_sock;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size; // used for accept()
	char s[INET6_ADDRSTRLEN];
	int numbytes; //using in receive or send

    char message_buf[100];  // temp saving place for receiving from client though TCP
    char result_buf[100]; // temp saving place for sending results to client though TCP
    char udp_send[50];
    char udp_received[MAXBUFLEN];  // buffer to receive map data from backend server
    char queryC[MAXBUFLEN];      // buffer to merge args from client and data from server A or B
    char *splited_args[4];
    char *parsed_results[5];    // {shortest_dist, trans_delay, prop_delay, total_delay, ans_path}

    int has_map_id = 0;
    int vtx_valid = 0;

	sockfd = setupTCP(PORTCLIENT);
    udp_sock = setupUDP2(UDPPORT);
	//printf("debug: sockfd is %d\n", sockfd);
	//printf("debug: sockfd_monitor is %d\n", sockfd_monitor);
	printf("The AWS is up and running.\n");
    
    printf("server: waiting for connections...\n");

    while(1) {
        has_map_id = 0;
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if(new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        if(!fork()) {
            close(sockfd);

            // receive arguments as single string
            if((numbytes = recv(new_fd, message_buf, sizeof message_buf, 0)) == -1) {
                perror("recv");
                exit(1);
            }
            message_buf[numbytes] = '\0';
            fflush(stdout);
            //printf("The received message is: %s\n", message_buf);

            // split received string of args into multiple string
            split_args2(splited_args, message_buf);
            printf("The AWS has received map ID %s, start vertex %s, destination vertex %s and file size %s from the client using TCP over port %s\n", splited_args[0], splited_args[1], splited_args[2], splited_args[3], UDPPORT);
            

            // merge map_id, source_vtx and target_vtx server
            sprintf(udp_send, "%s %s %s", splited_args[0], splited_args[1], splited_args[2]);

            // make a udp query from AWS to backend server A
            //udpQuery(udp_send, SERVERPORTA, udp_received);
            //printf("debug: udp sending %s\n", udp_send);
            udpQuery2(udp_sock, udp_send, SERVERPORTA, udp_received);
            //strcpy(udp_received, recvTmp);
            //printf("debug: udp received %lu\n", strlen(udp_received));

            // printf("debug: %s\n", udp_received);
            if(strcmp(udp_received, MAPNOTFOUND) != 0) {
                printf("The AWS has received map information from server A\n");
                has_map_id = 1;
            } else {
                //printf("debug: Try server B\n");

                // if map id cannot be found in server A, try server B
                memset(&udp_received, 0, sizeof udp_received);
                //udpQuery(udp_send, SERVERPORTB, udp_received);
                udpQuery2(udp_sock, udp_send, SERVERPORTB, udp_received);

                if(strcmp(udp_received, MAPNOTFOUND) != 0) {
                    printf("The AWS has received map information from server B\n");
                    has_map_id = 1;
                } else {
                    printf("debug: Ummm, no map id anywhere...\n");
                    has_map_id = 0;
                }
            }

            if(has_map_id == 1) {
                // As map has been found, check source (splited_args[1]) and target (splited_target[2]) are valid or not
                sprintf(queryC, "%s %s %s\n%s", splited_args[1], splited_args[2], splited_args[3], udp_received);
                vtx_valid = check_vtx(splited_args[1], splited_args[2], udp_received);
                switch (vtx_valid) {
                    // Source not found, target found
                    case 10:
                        sprintf(result_buf, "No vertex id %s found", splited_args[1]);
                        if (send(new_fd, result_buf, strlen(result_buf), 0) == -1) {
                            perror("send");
                        }
                        printf("Source vertex (%s) not found in the graph, sending error to client using TCP over port %s\n", splited_args[1], PORTCLIENT);
                        break;
                    
                    // Source found, target not found
                    case 1:
                        sprintf(result_buf, "No vertex id %s found", splited_args[2]);
                        if (send(new_fd, result_buf, strlen(result_buf), 0) == -1) {
                            perror("send");
                        }
                        printf("Target vertex (%s) not found in the graph, sending error to client using TCP over port %s\n", splited_args[2], PORTCLIENT);
                        break;
                    
                    // Source not found, target not found
                    case 0:
                        sprintf(result_buf, "No vetex id %s and %s found", splited_args[1], splited_args[2]);
                        if (send(new_fd, result_buf, strlen(result_buf), 0) == -1) {
                            perror("send");
                        }
                        printf("Both source (%s) and target (%s) not found in the graph, sending error to client using TCP over port %s\n", splited_args[1], splited_args[2], PORTCLIENT);
                        break;
                    
                    // Both found
                    default:
                        //printf("debug: Yeah! All found!\n");
                        //printf("debug: AWS send %s to server-C\n", queryC);
                        //udpQuery(queryC, SERVERPORTC, result_buf);
                        udpQuery2(udp_sock, queryC, SERVERPORTC, result_buf);
                        printf("The source and destination vertex are in the graph\n");
                        printf("The AWS has sent calculated results to client using TCP over port %s\n", PORTCLIENT);
                        parse_result(parsed_results, result_buf);
                        printf("The AWS has received the result from server C: \n");
                        printf("Shortest path: %s\n", parsed_results[4]);
                        printf("Shortest distance: %s\n", parsed_results[0]);
                        printf("Transmission delay: %s\n", parsed_results[1]);
                        printf("Propagation delay: %s\n", parsed_results[2]);
                        // rewrite the result buffer
                        //memset(&result_buf, 0, sizeof result_buf);
                        sprintf(result_buf, "%s\n%s\n%s\n%s\n%s", parsed_results[0], parsed_results[1], parsed_results[2], parsed_results[3], parsed_results[4]);
                        //printf("debug: sent %s back to client\n", result_buf);
                        if (send(new_fd, result_buf, strlen(result_buf), 0) == -1) {
                            perror("send");
                        }
                        printf("The AWS has sent calculated results to client using TCP over port %s\n", PORTCLIENT);
                        break;
                }
            } else {
                // Send error of no map to client
                sprintf(result_buf, "No map %s found", splited_args[0]);
                if (send(new_fd, result_buf, strlen(result_buf), 0) == -1) {
                    perror("send");
                }
            }
            
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }

    return 0;
}

