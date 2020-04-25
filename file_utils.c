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
#include <limits.h> 
#include <float.h>

#define IPADDRESS "127.0.0.1"  // local IP address
#define MYPORT "30484"	// the port used for UDP connection with AWS
#define MAPDIR "./maps/map1.txt" // file directory to get map information
#define MAXBUFLEN 5000
#define MAXNUMVERTICES 10

void search_map(char *map_id, char *map_data) {
    // a prepocess of map_id string: add newline symbol
    // char target[5];
    // sprintf(target, "%s\n", map_id);
    
    
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
            //printf("!!!\n");
            //printf("%s", line);
            //strcat(map_data, line);
            //strcat(data, "\n");
        }
    }

    if(map_id[0] != line[0]) {
        strcpy(map_data, "Map not found");
        return;
    } else {
        strcpy(map_data, line);
        while ((read = getline(&line, &len, file)) != -1) {
            if(isalpha(line[0]) != 0) {
                break;
            }
            // if(line[strlen(line)] == '\0') {
            //     printf("!!!\n");
            // }
            strcat(map_data, line);
        }
    }
}

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

// split received message
void split_args2(char *args[], char *message) {
    char *p = strtok (message, " ");
    int i = 0;
    while (p != NULL)
    {
        //printf("debug: first line parsing -- %s\n", p);
        args[i++] = p;
        p = strtok (NULL, " ");
    }
}

void parse_data(char *args[], char *data) {
    int source_flag = 0, target_flag = 0;
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
            char *token2 = strtok_r(token, " ", &end_token);
            int j = 0;
            while (token2 != NULL && j < 2) {
                token2 = strtok_r(NULL, " ", &end_token);
                j++;
            }
        }
        token = strtok_r(NULL, "\n", &end_str);
        i++;
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
                    } else if(vertices[k] == 0) {
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
    while(vertices[vtx_num] != 0 && vtx_num < MAXNUMVERTICES) {
        vtx_num++;
    }

    //printf("%d\n", vtx_num);
    return vtx_num;
}

int get_index(int *vtx_set, int vtx_num, int target) {
    for(int i = 0; i < vtx_num; i++) {
        if(vtx_set[i] == target)
            return i;
    }
    return -1;
}

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
                G[i][j] = atof(token2) / 100.0;
                G[j][i] = atof(token2) / 100.0;
            }
            token2 = strtok_r(NULL, " ", &end_token);
            token_idx++;
        }
        token = strtok_r(NULL, "\n", &end_str);
    }
}

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

int selectivePrintSolution(int target_idx, float dist[], int parent[]) 
{ 
    int src = 0; 
    printf("\nVertex\t Distance\tPath"); 
    printf("\n%d -> %d \t\t %.2f\t\t%d", src, target_idx, dist[target_idx], src); 
    printPath(parent, target_idx); 
    return 0;
}

// --------------------------------

void rec_path(int j, int parent[], int vtx_set[], char *path) 
{ 
      
    // Base Case : If j is source 
    if (parent[j] == -1) 
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
int dijkstra(int V, float graph[V][V], int src, int target, char *path, int *vtx_set) 
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
        parent[i] = -1; 
        dist[i] = FLT_MAX; 
        sptSet[i] = false; 
    } 

    printf("Parent \n");
    for(int i = 0; i < V; i++) {
        printf("%d\t", parent[i]);
    }
    printf("\n");
  
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
        // printf("debug: %d\n", u);
  
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
                printf("debug: assign parent[%d] as %d\n", v, u);
                dist[v] = dist[u] + graph[u][v]; 
            }  
    } 

    // printf("Parent \n");
    // for(int i = 0; i < V; i++) {
    //     printf("%d\t", parent[i]);
    // }
    // printf("\n");
  
    // print the constructed 
    // distance array 
    printSolution(src, dist, V, parent); 
    //selectivePrintSolution(target, dist, parent);

    get_path2(path, src, target, parent, vtx_set);

    return dist[target];
} 


int main(void) {
    char data[4000];
    search_map("O", data);
    // printf("%s", data);
    //printf("%c\n", data[strlen(data)-1]);
    //printf("%d\n", check_vtx("51", "100", data));

    char attached[5000];
    char *args[6];
    char edges[4000] = "";
    sprintf(attached, "%s %s %s\n%s", "80", "62", "1024", data);
    // parse_data(args, attached);

    int vtx_set[10] = {0}; // As the number of vertices in a single map, use 10-length array
    //float **G;
    int vtx_num = parse_data2(args, attached, edges, vtx_set);
    printf("%s %s %s %s %s %s\n", args[0], args[1], args[2], args[3], args[4], args[5]);

    printf("%s", edges);

    printf("the number of vertices: %d\n", vtx_num);

    float G[vtx_num][vtx_num];
    memset(G, 0, vtx_num * vtx_num * sizeof(float));
    add_edges(vtx_num, G, vtx_set, edges);

    // for(int i = 0; i < vtx_num; i++) {
    //     for(int j = 0; j < vtx_num; j++) {
    //         printf("%.2f\t", G[i][j]);
    //     }
    //     printf("\n");
    // }

    int source_idx = get_index(vtx_set, vtx_num, atoi(args[0]));
    int target_idx = get_index(vtx_set, vtx_num, atoi(args[1]));
    printf("Source vertex: %s, index: %d\n", args[0], source_idx);
    printf("The vertex set: \n");
    for(int i = 0; i < MAXNUMVERTICES; i++) {
        printf("%d\t", i);
    }
    printf("\n");
    for(int i = 0; i < MAXNUMVERTICES; i++) {
        printf("%d\t", vtx_set[i]);
    }
    printf("\n");
    char path_ans[50] = "";
    dijkstra(vtx_num, G, source_idx, target_idx, path_ans, vtx_set);
    printf("\n%s\n", path_ans);

}