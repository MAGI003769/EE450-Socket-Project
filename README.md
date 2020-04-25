# EE450 Socket Programming Project

This is the repository collects the code of 2020 Spring EE450 Socket  Programming Project. The project is C/C++ based and aims to simulate the client-server communication with TCP and UDP. 

## What I have done

I have already finished requires phases in description:

1. **Phase 1:** Implement a runnable client.
2. **Phase 2A:** Implement communication between client and AWS with TCP and communication between AWS ans backend server A/B. Enable AWS to check the availability of vertex IDs. Enable backend server A/B to search for map information.
3. **Phase 2B:** Implement the communication between AWS and server C. Implement Dijkstra Algorithm to find the shortest path. 
4. **Phase 3:** Finish the data transmission from server C to AWS and the result sending back to client. 



## Code Files and their fucntions

```
* client.c
Code for client to communicate with AWS by TCP. Transmit the request Map ID, Source ID, Target ID and file size read from command line. Request result from AWS.

* aws.c
Code for AWS which have following functions:
1. Receives the information from client with TCP.
2. Send Map ID to backend sever A/B for map information with UDP.
3. Check availability of Source ID and Target ID.
4. If available, send necessary information to backend server C with UDP.
5. Send client the result figured out by backend server C.

* serverA.c
Code for backend server A, connected with AWS by UDP, which receives the Map ID from AWS and searches if this ID is in the accessable map*.txt file. If available send the map information back to AWS. 

* serverB.c
Almost the same as serverA.c but the UDP port and directory of map*.txt file are different.  

* serverC.c
Code for backend server A, connected with AWS by UDP, which receives the necessary information to build a graph and find the shortest path using Dijkstra Algorithm. Send the result back to the AWS.
```



## Format of message exchange

Here I use the outputs from command `./client v 39 90 10124` as sample to show the format of message exchange.

```
1. Console output sample of client

The client is up and running.
The client has sent query to AWS using TCP: start vertex 39; destination vertex 90; map v; file size 10124
The client has received results from AWS: 
-------------------------------------------------------------------------------
Source	Destination	Min Length	Tt	Tp	Delay
-------------------------------------------------------------------------------
39		90	2676.19		0.01	0.03	0.04
-------------------------------------------------------------------------------
39 -- 47 -- 90



2. Console output sample of AWS

The AWS is up and running.
The AWS has received map ID v, start vertex 39, destination vertex 90 and file size 10124 from the client using TCP over port 33484
The AWS has sent map ID to server A using UDP over port 33484
The AWS has sent map ID to server B using UDP over port 33484
The AWS has received map information from server B
The AWS has sent map, source ID, destination ID, propagation speed and transmission speed to server C using UDP over port 33484
The source and destination vertex are in the graph
The AWS has sent calculated results to client using TCP over port 34484
The AWS has received the result from server C: 
Shortest path: 39 -- 47 -- 90
Shortest distance: 2676.19 km
Transmission delay: 0.01 s
Propagation delay: 0.03 s
The AWS has sent calculated results to client using TCP over port 34484


3. Console output sample of serverA

The Server A is up and running using UDP on port 30484
The Server A has received input for finding graph of map v
The Server A does not have the required graph id v
The Server A has sent "Graph not Found" to AWS


4. Console output sample of serverB

The Server B is up and running using UDP on port 31484
The Server B has received input for finding graph of map v
The Server B has sent Graph to AWS


5. Console output sample from serverC

The Server C is up and running using UDP on port 32484
The Server C has received data for calculation:
* Propagation speed: 101150.59 km/s;
* Transmission speed 908268 KB/s;
* map ID: v;
* Source ID: 39    Destination ID: 90;
The Server C has finished the calculation: 
Shortest path: 39 -- 47 -- 90
Shortest distance: 2676.1899
Transmission delay: 0.0100
Propagation delay: 0.0300
The Server C finished sening the output to AWS


```



## Idiosyncrasy in project

To handle the variable strings in C, there are lot of buffer with large size. The max length of buffers are set to 4000. If a single message or file exceeds this size, the program will crash.



## Resued Code

1. The implementation of TCP and UDP refers to **Beej’s Guide to Network Programming**. The variable names are quite similar. 
2. The implementation of Dijkstra Algorithm is based on [code from GeeksforGeeks](https://www.geeksforgeeks.org/dijkstras-shortest-path-algorithm-greedy-algo-7/) with some modifications. 



