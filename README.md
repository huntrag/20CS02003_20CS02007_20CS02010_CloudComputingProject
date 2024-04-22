# Cloud Computing Assignment
**Team Members:** <br>
Raghav Gade Nitin (20CS02003)  <br>
Om Saran    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  (20CS02007) <br>
Yatin Dhiman &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(20CS02010)

## Introduction to Lamport Clocks
A Lamport clock, named after computer scientist Leslie Lamport, is a simple logical clock algorithm used to determine the order of events in a distributed system. 

Instead of relying on physical time, Lamport clocks use a logical counter associated with each process. Each time an event occurs in a process, the Lamport clock for that process is incremented. When an event message is sent from one process to another, the Lamport clock value of the sender is included with the message. Upon receiving the message, the receiver updates its Lamport clock to be greater than the maximum of its current Lamport clock value and the received Lamport clock value plus one. This ensures that events are ordered according to the Lamport clock values. An example is given below for understanding.

<p align="center">
<img src="https://github.com/huntrag/lamport/assets/162877402/6fffdb74-4b81-4cfc-b49d-d43d96ac7818" alt="Alt text" width="400" height="200">
</p>

## Overview
The project implements a P2P netwrok in a distributed system using C++ and sockets. It consists of three components:
1. Sender <br>
2. Receiver <br>
3. Process (Shared Code)

## Components

### 1. Sender

The sender component is responsible for: 
* Connecting to peers 
* Sending registration and start messages 
* Handling critical section requests and replies

**Functions:**
* *connectToPeers(vector<int> &senderSocket, vector<int> &ports)* : Connects to peers using TCP sockets. It iterates over the provided ports and establishes a connection to each peer.
* *checkCritical(vector<int> &ports)* : Checks if all replies from peers are received. If so, checks if the current process can enter the critical section.
* *broadcast(vector<int> &endSockets, string &message)* : Broadcasts a message to all connected peers.
* *sendMessage(int socket, string &message)* : Sends a message to a specific socket.
* *checkStartStatus()* : Checks if all start confirmations from peers are received.
* *senderLoop(vector<int> &endSockets, vector<int> &ports)* : Main loop of the sender. Handles the registration, start, and critical section phases of the algorithm.
* *senderFn(int argc, char *argv[])* : Entry point of the sender component. Parses command-line arguments and initiates the sender loop.

### 2. Receiver

The receiver component is responsible for: 
* Binding to a port and listening for incoming connections 
* Receiving messages from sender processes 
* Handling various types of messages: registration, request, reply, release, and start 

**Functions:**
* *bind(int &receiverSocket, int port)* : Binds the receiver socket to the specified port. It sets socket options and binds the socket to the port.
* *checkMessage(int ind, string &message)* : Parses received messages and performs actions based on the message type.
The message format consists of a flag followed by optional parameters, delimited by '#' or '$', and terminated by '|'. The flag indicates the message type:<br><br>
0 (REQ): Handles request messages by adding them to the priority queue.<br>
1 (REPLY): Processes reply messages and updates reply status.<br>
2 (REL): Removes elements from the priority queue.<br>
3 (REG): Handles registration messages by updating port mappings.<br>
4 (START): Processes start confirmation messages and updates start status.<br>
For example, a request message might look like 0#8082$456|, where 8082 is the requesting port and 456 is the timestamp.
	
* *serve(int master_socket, int port)* : Main function for handling incoming connections and messages. It uses the `select` function to monitor multiple sockets for activity, accepts incoming connections, and processes incoming data.
* *receiverFn(int port)* : It sets up a receiver socket, binds it to a specified port, and then enters a loop to continuously listen for incoming connections and messages from sender peers in a distributed system.


### 3.Process
The process component contains shared data structures and functions used by both sender and receiver components.

**Functions:**
* *criticalSection()* : Simulates a critical section, waits for 3 seconds and then exits the critical section.
* *removeFromPQ(int port)* : Removes elements from the priority queue (pq) based on a specified port. Uses a mutex (mex) to ensure that only one thread can access the critical section at a time.


## How to Run

1. Compile the sender, receiver, and shared code files. <br> <pre> ./receiver <port_number> </pre>
2. Run the receiver with the desired port number: <br> <pre> ./sender <receiver_ip> < port1 > < port2 > < port3 > </pre>




