#include <thread>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <mutex>
#include <vector>
#include <stack>
#include <queue>
#include <map>
// #include <receiver.hpp>
// #include <sender.hpp>

using namespace std;
// Global Variables
int maxPeer;
static int counter = 1;
bool connected = false;
vector<bool> replyStatus;
vector<bool> startStatus;
bool requested = false;
bool registered = false;
bool arrived = true;
bool cs = false;
int receiverPort;
priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
queue<int> replyQueue;
map<int, int> ind2Port;
map<int, int> port2ind;
mutex mex;

struct socketAddr
{
    int port;
    sockaddr_in receiverAddress;
    string ipaddress;

    socketAddr(int port)
    {
        bzero((char *)&receiverAddress, sizeof(receiverAddress));
        this->port = port;
        receiverAddress.sin_family = AF_INET;
        receiverAddress.sin_port = htons(port);
        receiverAddress.sin_addr.s_addr = INADDR_ANY;
    }

    socketAddr(int port, string ipaddress)
    {
        bzero((char *)&receiverAddress, sizeof(receiverAddress));
        this->port = port;
        receiverAddress.sin_family = AF_INET;
        receiverAddress.sin_port = htons(port);
        receiverAddress.sin_addr.s_addr = inet_addr((const char *)ipaddress.c_str());
        // receiverAddress.sin_addr.s_addr = inet_addr("192.168.1.100");
    }
};

void connectToPeers(vector<int> &senderSocket, vector<int> &ports)
{
    vector<socketAddr *> connArr(ports.size());
    cout << ports.size() << endl;
    queue<int> connectSockets;
    for (int i = 0; i < ports.size(); i++)
    {
        connArr[i] = new socketAddr(ports[i]);
        senderSocket[i] = socket(AF_INET, SOCK_STREAM, 0);
        connectSockets.push(i);
    }
    while (!connectSockets.empty())
    {
        int ind = connectSockets.front();
        connectSockets.pop();
        if (connect(senderSocket[ind], (struct sockaddr *)&(connArr[ind]->receiverAddress), sizeof(connArr[ind]->receiverAddress)) == 0)
        {
            cout << "(sender Thread) Connected to " << connArr[ind]->port << endl;
        }
        else
        {
            cout << "(sender Thread) No response from " << connArr[ind]->port << endl;
            connectSockets.push(ind);
        }
        sleep(1);
    }
    if (connectSockets.empty())
    {
        connected = true;
    }
    replyStatus.resize(maxPeer);
    startStatus.resize(maxPeer);
    cout << "Everybody connected " << connected << endl;
}

void removeFromPQ(int port)
{
    mex.lock();
    stack<pair<int, int>> stk;
    while (!pq.empty())
    {
        pair<int, int> cur = pq.top();
        pq.pop();
        if (cur.second == port)
        {
            break;
        }
        else
        {
            stk.push(cur);
        }
    }
    while (!stk.empty())
    {
        pq.push(stk.top());
        stk.pop();
    }
    mex.unlock();
}

bool checkCritical(vector<int> &ports)
{
    mex.lock();
    for (int i = 0; i < maxPeer; i++)
    {
        if (!replyStatus[i])
        {
            cout << "Reply from port pending with port: " << ports[i] << endl;
            mex.unlock();
            return false;
        }
    }
    pair<int, int> cur = pq.top();
    if (cur.second == receiverPort)
    {
        cs = true;
    }
    mex.unlock();
    return true;
}

void broadcast(vector<int> &endSockets, string &message)
{
    message.push_back('|');
    char buffer[message.length()];
    memset(buffer, '\0', message.length());
    strcpy(buffer, &message[0]);
    // cout << "Sent message (" << message << ") to broadcast" << endl;
    for (auto &endSocket : endSockets)
    {
        mex.lock();
        send(endSocket, &buffer[0], message.length(), 0);
        mex.unlock();
    }
}

void sendMessage(int socket, string &message)
{
    message.push_back('|');
    char buffer[message.length()];
    memset(buffer, '\0', message.length());
    strcpy(buffer, &message[0]);
    mex.lock();
    send(socket, &buffer[0], message.length(), 0);
    mex.unlock();
}

void criticalSection()
{
    cout << "Entered Section\n";
    sleep(3);
    cout << "Exit Section\n";
}

bool checkStartStatus()
{
    mex.lock();
    for (int i = 0; i < maxPeer; i++)
    {
        if (!startStatus[i])
        {
            mex.unlock();
            return false;
        }
    }
    mex.unlock();
    return true;
}

void senderLoop(vector<int> &endSockets, vector<int> &ports)
{
    while (true)
    {
        if (!connected)
        {
            sleep(2);
            continue;
        }

        if (!registered)
        {
            string message = "3#" + to_string(receiverPort);
            broadcast(endSockets, message);
            cout << "Sent registration broadcast as message: " << message << endl;
            registered = true;
        }

        if (ind2Port.size() < maxPeer)
        {
            sleep(1);
            continue;
        }

        if (arrived)
        {
            cout << "Everybody's registrations arrived" << endl;
            for (auto &r : ind2Port)
            {
                cout << "(" << r.first << ", " << r.second << "), ";
            }
            cout << endl;
            arrived = false;
            string message = "4#" + to_string(receiverPort);
            broadcast(endSockets, message);
            cout << "Sent start broadcast as message: " << message << endl;
        }

        if (!checkStartStatus())
        {
            cout << "Everybody's start confirmation yet to arrive" << endl;
            sleep(2);
            continue;
        }

        if (requested)
        {
            if (checkCritical(ports))
            {
                // Start Critical Section
                criticalSection();
                string message = "2#" + to_string(receiverPort);
                requested = false;
                cs = false;
                fill(replyStatus.begin(), replyStatus.end(), false);
                broadcast(endSockets, message);
                cout << "Sent release broadcast as message: " << message << endl;
            }
        }

        while (!replyQueue.empty())
        {
            string message = "1#" + to_string(receiverPort);
            sendMessage(endSockets[port2ind[replyQueue.front()]], message);
            cout << "Sent REPLY to port " << replyQueue.front() << " as message: " << message << endl;
            replyQueue.pop();
        }
        int randNum = 5;
        if (!requested)
        {
            randNum = (rand() % 5);

            cout << "Random Number " << randNum << endl;
        }

        if (registered && randNum < 2 && !requested)
        {
            requested = true;
            string message = "0#" + to_string(receiverPort) + "$" + to_string(counter);
            mex.lock();
            pq.push(make_pair(counter, receiverPort));
            mex.unlock();
            broadcast(endSockets, message);
            cout << "Sent REQUEST broadcast as message: " << message << endl;
            counter++;
        }
        sleep(2);
    }
}

void senderFn(int argc, char *argv[])
{
    // sender
    if (argc < 3)
    {
        return;
    }
    vector<int> senderSockets;
    vector<int> ports;

    ports.resize(argc - 2);
    for (int i = 2; i < argc; i++)
    {
        ports[i - 2] = atoi(argv[i]);
        cout << ports[i - 2] << endl;
        port2ind[ports[i - 2]] = i - 2;
    }
    senderSockets.resize(ports.size());

    connectToPeers(senderSockets, ports);

    senderLoop(senderSockets, ports);

    for (auto &senderSocket : senderSockets)
    {
        close(senderSocket);
    }
}

bool bind(int &receiverSocket, int port)
{
    int iOptionValue = 1;
    if (setsockopt(receiverSocket, SOL_SOCKET, SO_REUSEADDR, &iOptionValue, sizeof(int)) == -1)
    {
        printf("(receiver Thread) ERROR setting socket options.\n");
        return false;
    }

    sockaddr_in receiverAddress;
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_port = htons(port);
    receiverAddress.sin_addr.s_addr = INADDR_ANY;

    bzero((char *)&receiverAddress, sizeof(receiverAddress));

    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.s_addr = INADDR_ANY;
    receiverAddress.sin_port = htons(port);

    if (bind(receiverSocket, (struct sockaddr *)&receiverAddress, sizeof(receiverAddress)) < 0)
    {
        return false;
    }

    printf("(receiver Thread) receiver is listening to %i.\n", port);
    listen(receiverSocket, 5);

    // Add the Non-Blocking param to the socket.

    return true;
}

void checkMessage(int ind, string &message)
{
    // 0 for REQ
    // 1 for REPLY
    // 2 for REL
    // 3 for REG
    // 4 for START
    int flag = (message[0] - '0');
    int hashPos = message.find_first_of('#');
    int dollarPos = message.find_first_of('$');
    if (flag == 3)
    {
        int port = stoi(message.substr(hashPos + 1, 4));
        cout << "Got registration message as: " << message << endl;
        ind2Port[ind] = port;
    }
    else if (flag == 0)
    {
        // ind is requesting
        int timestamp = stoi(message.substr(dollarPos + 1));
        int port = stoi(message.substr(hashPos + 1, dollarPos - hashPos - 1));
        cout << "Got receive message from " << port << " as: " << message << endl;
        replyQueue.push(port);
        mex.lock();
        pq.push(make_pair(timestamp, port));
        mex.unlock();
    }
    else if (flag == 1)
    {
        mex.lock();
        int port = stoi(message.substr(hashPos + 1));
        cout << "Got reply from " << port << " as message: " << message << endl;
        replyStatus[port2ind[port]] = true;
        mex.unlock();
    }
    else if (flag == 2)
    {
        int port = stoi(message.substr(hashPos + 1));
        removeFromPQ(port);
    }
    else if (flag == 4)
    {
        mex.lock();
        int port = stoi(message.substr(hashPos + 1));
        cout << "Got start confirmation from " << port << " as message: " << message << endl;
        startStatus[port2ind[port]] = true;
        mex.unlock();
    }
    else
    {
        cout << "Invalid message: " << message << endl;
    }
}

void serve(int master_socket, int port)
{
    int i, sd, new_socket, addrlen, activity;
    int max_sd;
    fd_set readfds;
    struct sockaddr_in address;

    vector<int> endSockets(maxPeer);

    while (1)
    {
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        // add child sockets to set
        for (i = 0; i < maxPeer; i++)
        {
            // socket descriptor
            sd = endSockets[i];
            // if valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);
            // highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        // wait for an activity on one of the sockets , timeout is NULL ,
        // so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            printf("(receiver Thread) select error\n");
        }

        // If something happened on the master socket ,
        // then its an incoming connection
        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket,
                                     (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // inform user of socket number - used in send and receive commands
            printf("(receiver Thread) New connection ,socket fd is %d ,ip is : %s ,port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            for (i = 0; i < maxPeer; i++)
            {
                if (endSockets[i] == 0)
                {
                    endSockets[i] = new_socket;
                    break;
                }
            }
        }

        for (int i = 0; i < maxPeer; i++)
        {
            char buffer[1025] = {0};
            sd = endSockets[i];
            if (FD_ISSET(sd, &readfds))
            {
                int valread;
                if ((valread = read(sd, buffer, 1024)) == 0)
                {
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("(receiver Thread) Host disconnected , ip %s , port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    endSockets[i] = 0;
                }
                else
                {
                    string message(buffer);
                    int cur;
                    while ((cur = message.find_first_of('|')) < message.length())
                    {
                        string mess = message.substr(0, cur);
                        cout << "Received message: " << mess << endl;
                        message = message.substr(cur + 1);
                        checkMessage(i, mess);
                    }
                }
            }
        }
    }
}

void receiverFn(int port)
{
    // receiver
    int receiverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (!bind(receiverSocket, port))
    {
        cout << "Error on binding to port" << endl;
    }

    // int senderSocket = accept(receiverSocket, nullptr, nullptr);

    // fcntl(receiverSocket, F_SETFL, fcntl(receiverSocket, F_GETFL) | O_NONBLOCK);

    // char buffer[1024] = {0};
    // recv(senderSocket, buffer, sizeof(buffer), 0);
    // cout << "Message from sender: " << senderSocket << buffer << endl;
    serve(receiverSocket, port);

    close(receiverSocket);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    maxPeer = argc - 2;
    receiverPort = atoi(argv[1]);
    thread sender(senderFn, argc, argv);
    thread receiver(receiverFn, receiverPort);

    sender.join();
    receiver.join();

    return 0;
}