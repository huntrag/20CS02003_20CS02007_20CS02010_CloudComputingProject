# Cloud Computing Assignment

**Team Members:** <br>
Raghav Nitin Gade (20CS02003) <br>
Om Saran &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; (20CS02007) <br>
Yatin Dhiman &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(20CS02010)

## Overview

The project implements a P2P network in a distributed system using C++ and sockets. It consists of three components:

1. Sender <br>
2. Receiver <br>
3. Process (Shared Code)

There are 2 versions of the project:

1. One that connects to ports via loopback addresses
2. One that connects other computers.

Directory Structure

1. drafts contain the previous versions executables for checpoint
2. final_version_1 connects locally through ports
3. final_version_2 connects through computers, please make sure you disable firewalls and add ipaddress according to ifconfig
4. imgs contain the relevant images
5. unstrutured contains the code before OOPs implementation

- **This code can be genrealised for more than 3 peers.** <br>
- **This code forms a complete peers network where P1 can talk to both P2 and P3.**
- **The implementation uses select() in the receiver thread to check the file descriptor and recv() the code from other senders. This ensures that we don't have to connect to senders via different threads**
- **The timestamp counter only increments on REQ messages as those are the only significant events considered.**
- **Implementation is using the OOPs Paradigm. There are 3 classes - Process, Sender and Receiver. Each contains its own set of properties and methods. Sender and Receiver gets the copy of Process to modify. Sender inherits the Receiver class**
  ![image](https://github.com/huntrag/20CS02003_20CS02007_20CS02010_CloudComputingProject/assets/162877402/ccebe865-6c4e-4288-8415-761997e46485)

## Output Screenshot

Below is the screenshot of the ran lmaport's mutual exclusion program.
![image](https://github.com/huntrag/20CS02003_20CS02007_20CS02010_CloudComputingProject/assets/162877402/b9c45748-a291-42bf-bd1c-1308164183e9)

In order to compile<br>

<pre>g++ process.hpp receiver.hpp sender.hpp index.cpp -o test</pre>

In order to run the 2 versions of the program, below is the format given:

1. Local version (This will ensure connection of 3 peers, to add in, increase the number of ports). Create 3 seperate processes and change the server ports alternatively. <br>
Syntax: ./test [Server_port] [Other_Port1] [Other_Port2]
eg.
<pre>./test 8080 8081 8082
./test 8081 8082 8080
./test 8082 8080 8081</pre>

2. Other version that connects other computer takes in ip addresses of all the peers. <br>
   Syntax: ./test [Server_port] [Server_Ip] [Other_Port1] [Other_Ip1] [Other_Port2] [Other_Ip2]

## Handling the Critical Section part

Each program in its turn takes in the first line of the text file and add it to a random number and appends the log at the end of the file along with the timestamp.<br>
![image](https://github.com/huntrag/20CS02003_20CS02007_20CS02010_CloudComputingProject/assets/162877402/9e8654f9-408f-4341-869a-25e08eb679d9)

## Introduction to Lamport Clocks

A Lamport clock, named after computer scientist Leslie Lamport, is a simple logical clock algorithm used to determine the order of events in a distributed system.

Instead of relying on physical time, Lamport clocks use a logical counter associated with each process. Each time an event occurs in a process, the Lamport clock for that process is incremented. When an event message is sent from one process to another, the Lamport clock value of the sender is included with the message. Upon receiving the message, the receiver updates its Lamport clock to be greater than the maximum of its current Lamport clock value and the received Lamport clock value plus one. This ensures that events are ordered according to the Lamport clock values. An example is given below for understanding.

<p align="center">
<img src="https://github.com/huntrag/lamport/assets/162877402/6fffdb74-4b81-4cfc-b49d-d43d96ac7818" alt="Alt text" width="400" height="200">
</p>

## Components

### 1. Sender

The sender component is responsible for:

- Connecting to peers
- Sending registration and start messages
- Handling critical section requests and replies

**Functions:**

- _connectToPeers(vector<int> &senderSocket, vector<int> &ports)_ : Connects to peers using TCP sockets. It iterates over the provided ports and establishes a connection to each peer.
- _checkCritical(vector<int> &ports)_ : Checks if all replies from peers are received. If so, checks if the current process can enter the critical section.
- _broadcast(vector<int> &endSockets, string &message)_ : Broadcasts a message to all connected peers.
- _sendMessage(int socket, string &message)_ : Sends a message to a specific socket.
- _checkStartStatus()_ : Checks if all start confirmations from peers are received.
- _senderLoop(vector<int> &endSockets, vector<int> &ports)_ : Main loop of the sender. Handles the registration, start, and critical section phases of the algorithm.
- *senderFn(int argc, char *argv[])\* : Entry point of the sender component. Parses command-line arguments and initiates the sender loop.

### 2. Receiver

The receiver component is responsible for:

- Binding to a port and listening for incoming connections
- Receiving messages from sender processes
- Handling various types of messages: registration, request, reply, release, and start

**Functions:**

- _bind(int &receiverSocket, int port)_ : Binds the receiver socket to the specified port. It sets socket options and binds the socket to the port.
- _checkMessage(int ind, string &message)_ : Parses received messages and performs actions based on the message type.
  The message format consists of a flag followed by optional parameters, delimited by '#' or '$', and terminated by '|'. The flag indicates the message type:<br><br>
  0 (REQ): Handles request messages by adding them to the priority queue.<br>
  1 (REPLY): Processes reply messages and updates reply status.<br>
  2 (REL): Removes elements from the priority queue.<br>
  3 (REG): Handles registration messages by updating port mappings.<br>
  4 (START): Processes start confirmation messages and updates start status.<br>
  For example, a request message might look like 0#8082$456|, where 8082 is the requesting port and 456 is the timestamp.
- _serve(int master_socket, int port)_ : Main function for handling incoming connections and messages. It uses the `select` function to monitor multiple sockets for activity, accepts incoming connections, and processes incoming data.
- _receiverFn(int port)_ : It sets up a receiver socket, binds it to a specified port, and then enters a loop to continuously listen for incoming connections and messages from sender peers in a distributed system.

### 3.Process

The process component contains shared data structures and functions used by both sender and receiver components.

**Functions:**

- _criticalSection()_ : Simulates a critical section, waits for 3 seconds and then exits the critical section.
- _removeFromPQ(int port)_ : Removes elements from the priority queue (pq) based on a specified port. Uses a mutex (mex) to ensure that only one thread can access the critical section at a time.
