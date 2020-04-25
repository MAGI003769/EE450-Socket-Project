/**
**
** serverC.c
** Author: Ruihao Wang
** USC-ID#: 9867439484
**
** Code is modified and expanded base on stream server source in beej
** Specificly, backend server implementation refers to listener.c
**
**
** TODO: 
** 1. communication with aws
** 2. process data in correct format
** 3. implement Dijkstra’s shortest path algorithm
** 4. calculate delay
**
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
#include <stdbool.h>
#include <float.h>
#include <math.h>

#define IPADDRESS "127.0.0.1"  // local IP address
#define MYPORT "32484"	// the port used for UDP connection with AWS
#define MAPDIR "./maps/map2.txt" // file directory to get map information
#define MAXBUFLEN 4000
#define MAPNOTFOUND "Map not found"
#define MAXNUMVERTICES 15 // As the number of vertices in a single map, use 10-length array

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

int parse_data2(char *args[], char *data, char *edges, int *vertices) {
    char *end_str;
    char *token = strtok_r(data, "\n", &end_str);
    int i = 0;

    while (token != NULL) {
        char *end_token;
        //printf("debug: check line -- %s\n", token);
        if(i == 0) {
            split_args2(args, token);
        } else if (i < 4) {
            args[i+2] = token;
        } else {
            strcat(edges, token);
            strcat(edges, "\n");
            char *token2 = strtok_r(token, " ", &end_token);
            int j = 0;
            while (token2 != NULL && j < 2) {
                for(int k = 0; k < MAXNUMVERTICES; k++) {
                    if(vertices[k] == atoi(token2)) {
                        break;
                    } else if(vertices[k] == -1) {
                        vertices[k] = atoi(token2);
                        break;
                    }
                }
                token2 = strtok_r(NULL, " ", &end_token);
                j++;
            }
        }
        token = strtok_r(NULL, "\n", &end_str);
        i++;
    }

    int vtx_num = 0;
    while(vertices[vtx_num] != -1 && vtx_num < MAXNUMVERTICES) {
        vtx_num++;
    }
    return vtx_num;
}

// function to get index of vertex in graph
int get_index(int *vtx_set, int vtx_num, int target) {
    for(int i = 0; i < vtx_num; i++) {
        if(vtx_set[i] == target)
            return i;
    }
    return -1;
}

// function to add edges to empty graph G[vtx_num][vtx_num]
void add_edges(int vtx_num, float G[][vtx_num], int *vtx_set, char *edges) {
    char *end_str;
    char *token = strtok_r(edges, "\n", &end_str);
    int i = 0, j = 0;

    while (token != NULL) {
        char *end_token;
        //printf("debug: check line -- %s\n", token);
        int token_idx = 0;
        char *token2 = strtok_r(token, " ", &end_token);
        while (token2 != NULL) {
            if(token_idx == 0) {
                i = get_index(vtx_set, vtx_num, atoi(token2));
            } else if(token_idx == 1) {
                j = get_index(vtx_set, vtx_num, atoi(token2));
            } else {
                //printf("debug: assign edge (%d, %d): %f\n", i, j, atof(token2));
                // Assign the edge value twice as the graph is undirected
                G[i][j] = atof(token2);
                G[j][i] = atof(token2);
            }
            token2 = strtok_r(NULL, " ", &end_token);
            token_idx++;
        }
        token = strtok_r(NULL, "\n", &end_str);
    }
}

/*
 *
 * Following funcitons are related to Dijkstra Algorithm to find shortest path
 * 
 */

// A utility function to find the vertex with minimum distance value, from 
// the set of vertices not yet included in shortest path tree 
int minDistance(float dist[], bool sptSet[], int V) 
{ 
    // Initialize min value 
    float min = FLT_MAX, min_index; 
  
    for (int v = 0; v < V; v++) 
        if (sptSet[v] == false && dist[v] <= min) 
            min = dist[v], min_index = v; 
  
    return min_index; 
}

void printPath(int parent[], int j) 
{ 
      
    // Base Case : If j is source 
    if (parent[j] == -1) 
        return; 
  
    printPath(parent, parent[j]); 
  
    printf(" -- %d", j); 
} 
  
// A utility function to print  
// the constructed distance 
// array 
int printSolution(int src, float dist[], int V, int parent[]) 
{ 
    //int src = 0; 
    printf("Vertex\t Distance\tPath"); 
    for (int i = 0; i < V; i++) 
    { 
        printf("\n%d -> %d \t\t %.2f\t\t%d", 
                      src, i, dist[i], src); 
        printPath(parent, i); 
    } 
    return 0;
}

void rec_path(int j, int parent[], int vtx_set[], char *path) 
{ 
      
    // Base Case : If j is source 
    if (parent[j] == - 1) 
        return; 
  
    rec_path(parent[j], parent, vtx_set, path); 

    char tmp[20] = "";
    sprintf(tmp, " -- %d", vtx_set[j]);
    strcat(path, tmp);
}

int get_path2(char *path, int source_idx, int target_idx, int parent[], int vtx_set[]) 
{ 
    sprintf(path, "%d", vtx_set[source_idx]);
    rec_path(target_idx, parent, vtx_set, path);
    return 0;
}

// Funtion that implements Dijkstra's 
// single source shortest path 
// algorithm for a graph represented 
// using adjacency matrix representation 
float dijkstra(int V, float graph[V][V], int src, int target, char *path, int *vtx_set) 
{ 
      
    // The output array. dist[i] 
    // will hold the shortest 
    // distance from src to i 
    float dist[V];  
  
    // sptSet[i] will true if vertex 
    // i is included / in shortest 
    // path tree or shortest distance  
    // from src to i is finalized 
    bool sptSet[V]; 
  
    // Parent array to store 
    // shortest path tree 
    int parent[V]; 
  
    // Initialize all distances as  
    // INFINITE and stpSet[] as false 
    for (int i = 0; i < V; i++) 
    { 
        parent[src] = -1; 
        dist[i] = FLT_MAX; 
        sptSet[i] = false; 
    } 
  
    // Distance of source vertex  
    // from itself is always 0 
    dist[src] = 0; 
  
    // Find shortest path 
    // for all vertices 
    for (int count = 0; count < V - 1; count++) 
    { 
        // Pick the minimum distance 
        // vertex from the set of 
        // vertices not yet processed.  
        // u is always equal to src 
        // in first iteration. 
        int u = minDistance(dist, sptSet, V); 
  
        // Mark the picked vertex  
        // as processed 
        sptSet[u] = true; 
  
        // Update dist value of the  
        // adjacent vertices of the 
        // picked vertex. 
        for (int v = 0; v < V; v++) 
  
            // Update dist[v] only if is 
            // not in sptSet, there is 
            // an edge from u to v, and  
            // total weight of path from 
            // src to v through u is smaller 
            // than current value of 
            // dist[v] 
            if (!sptSet[v] && graph[u][v] && 
                dist[u] + graph[u][v] < dist[v]) 
            { 
                parent[v] = u; 
                dist[v] = dist[u] + graph[u][v]; 
            }  
    } 
  
    // print the constructed 
    // distance array 
    //printSolution(src, dist, V, parent); 
    //selectivePrintSolution(target, dist, parent);

    get_path2(path, src, target, parent, vtx_set);

    return dist[target];
} 


int main(void) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    char data[MAXBUFLEN]; // 
    char buf[MAXBUFLEN];  // 
    char edges[MAXBUFLEN] = "";
    char *args[6];  // {<source ID>, <tarrget ID>, <file size>, <MAP ID>, <Prop speed>, <Trans speed>}

    int vtx_set[MAXNUMVERTICES];
    // set all vertex initialed as -1 to prevent the 0-initialization have crash for vertex id 0
    int vtx_num = 0;
    int src_idx, target_idx;
    char ans_path[100] = "";
    float shortest_dist = FLT_MAX;
    float trans_delay = 0.0;
    float prop_delay = 0.0;
    float total_delay = 0.0;

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
    
    printf("The Server C is up and running using UDP on port %s\n", MYPORT);
    fflush(stdout);

    while(1) {
        if((numbytes = recvfrom(sockfd, data, MAXBUFLEN-1, 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        data[numbytes] = '\0';
        // printf("debug: server-C received %d bytes from AWS\n", numbytes);
        // printf("debug: server-C received %s bytes from AWS\n", data);

        // read file and search the map_id
        //memset(&buf, 0, sizeof buf);
        memset(&edges, 0, sizeof edges);   // REMEBER to flush the edges buffer !!!
        for(int i = 0; i < MAXNUMVERTICES; i++) { // REMEBER to reset the vtx_set for new query
            vtx_set[i] = -1;
        }
        vtx_num = parse_data2(args, data, edges, vtx_set);
        // printf("debug: the number of vertices in map is %d\n", vtx_num);
        // for(int i = 0; i < MAXNUMVERTICES; i++) {
        //     printf("%d\t", vtx_set[i]);
        // }
        // printf("\n");
        // printf("debug: the length of edges in map is %lu\n", strlen(edges));
        // printf("debug: Edges:\n %s\n", edges);

        printf("The Server C has received data for calculation:\n");
        printf("* Propagation speed: %s km/s;\n", args[4]);
        printf("* Transmission speed %s KB/s;\n", args[5]);
        printf("* map ID: %s;\n", args[3]);
        printf("* Source ID: %s    Destination ID: %s;\n", args[0], args[1]);

        // printf("debug: vtx_set\n");
        // for(int i = 0; i < vtx_num; i++) {
        //     printf("%d\t", vtx_set[i]);
        // }
        // printf("\n");

        // Construct graph represents by a matrix initialized 0
        float G[vtx_num][vtx_num];
        memset(G, 0, vtx_num * vtx_num * sizeof(float));
        add_edges(vtx_num, G, vtx_set, edges);

        // Convert vertex IDs to index in graph matrix and find the shortest path
        src_idx = get_index(vtx_set, vtx_num, atoi(args[0]));
        target_idx = get_index(vtx_set, vtx_num, atoi(args[1]));
        shortest_dist = dijkstra(vtx_num, G, src_idx, target_idx, ans_path, vtx_set);

        // calculate delays
        trans_delay = atof(args[2]) / atof(args[5]);
        prop_delay = shortest_dist / atof(args[4]);
        total_delay = trans_delay + prop_delay;

        // printf("debug: The vertex set: \n");
        // for(int i = 0; i < MAXNUMVERTICES; i++) {
        //     printf("%d\t", get_index(vtx_set, vtx_num, vtx_set[i]));
        // }
        // printf("\n");
        // for(int i = 0; i < MAXNUMVERTICES; i++) {
        //     printf("%d\t", vtx_set[i]);
        // }
        // printf("\n");

        printf("The Server C has finished the calculation: \n");
        printf("Shortest path: %s\n", ans_path);
        printf("Shortest distance: %.4f\n", shortest_dist);
        printf("Transmission delay: %.4f\n", trans_delay);
        printf("Propagation delay: %.4f\n", prop_delay);

        // printing adjacency matrix for debug
        // for(int i = 0; i < vtx_num; i++) {
        //     for(int j = 0; j < vtx_num; j++) {
        //         printf("%.2f\t", G[i][j]);
        //     }
        //     printf("\n");
        // }

        // write answers to buf[] to send back to
        sprintf(buf, "%.4f\n%.4f\n%.4f\n%.4f\n%s", shortest_dist, trans_delay, prop_delay, total_delay, ans_path);

        numbytes = sendto(sockfd, buf, strlen(buf), 0,
            (struct sockaddr *)&their_addr, addr_len);
        
        if(numbytes == -1) {
            perror("listener: sendto");
            exit(1);
        }

        printf("The Server C finished sening the output to AWS\n");
        
        //close(sockfd);
    }

    return 0;

}

