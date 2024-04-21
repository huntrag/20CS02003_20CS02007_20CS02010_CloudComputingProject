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

using namespace std;
// Global Variables
int maxPeer;
static int counter = 1;
bool connected = false;
vector<bool> replyStatus;
bool requested = false;
bool registered = false;
bool arrived = false;
bool cs = false;
int serverPort;
priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
queue<int> replyQueue;
map<int, int> ind2Port;
map<int, int> port2ind;
mutex mex;

struct connStruct
{
    int port;
    sockaddr_in serverAddress;

    connStruct(int port)
    {
        this->port = port;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = INADDR_ANY;
    }
};

void connectToPeers(vector<int> &clientSocket, vector<int> &ports)
{
    vector<connStruct *> connArr(ports.size());
    cout << ports.size() << endl;
    queue<int> connectSockets;
    for (int i = 0; i < ports.size(); i++)
    {
        connArr[i] = new connStruct(ports[i]);
        clientSocket[i] = socket(AF_INET, SOCK_STREAM, 0);
        connectSockets.push(i);
    }
    while (!connectSockets.empty())
    {
        int ind = connectSockets.front();
        connectSockets.pop();
        if (connect(clientSocket[ind], (struct sockaddr *)&(connArr[ind]->serverAddress), sizeof(connArr[ind]->serverAddress)) == 0)
        {
            cout << "(Client Thread) Connected to " << connArr[ind]->port << endl;
        }
        else
        {
            cout << "(Client Thread) No response from " << connArr[ind]->port << endl;
            connectSockets.push(ind);
        }
        sleep(1);
    }
    if (connectSockets.empty())
    {
        connected = true;
    }
    replyStatus.resize(maxPeer);
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
    for (int i = 0; i < maxPeer; i++)
    {
        if (!replyStatus[i])
        {
            cout << "Reply from port pending with ind " << ports[i] << endl;
            return false;
        }
    }
    pair<int, int> cur = pq.top();
    if (cur.second == serverPort)
    {
        cs = true;
    }
    return true;
}

void broadcast(vector<int> &endSockets, string &message)
{
    char buffer[message.length()];
    memset(buffer, '\0', message.length());
    strcpy(buffer, &message[0]);
    // cout << "Sent message (" << message << ") to broadcast" << endl;
    for (auto &endSocket : endSockets)
    {
        send(endSocket, &buffer[0], message.length(), 0);
    }
}

void sendMessage(int socket, string &message)
{
    char buffer[message.length()];
    memset(buffer, '\0', message.length());
    strcpy(buffer, &message[0]);

    send(socket, &buffer[0], message.length(), 0);
}

void criticalSection()
{
    cout << "Entered Section\n";
    sleep(1);
    cout << "Exit Section\n";
}

void clientLoop(vector<int> &endSockets, vector<int> &ports)
{
    while (true)
    {
        if (!connected)
        {
            sleep(0.1);
            continue;
        }

        if (!registered)
        {
            string message = "3" + to_string(serverPort);
            broadcast(endSockets, message);
            cout << "Sent registration broadcast as message: " << message << endl;
            registered = true;
        }

        if (ind2Port.size() < maxPeer - 1)
        {
            sleep(0.1);
            continue;
        }

        if (arrived)
        {
            cout << "Everybody's registrations arrived" << endl;
            arrived = false;
        }

        if (requested)
        {
            if (checkCritical(ports))
            {
                // Start Critical Section
                criticalSection();
                string message = "2";
                requested = false;
                cs = false;
                fill(replyStatus.begin(), replyStatus.end(), false);
                broadcast(endSockets, message);
                cout << "Sent release broadcast as message: " << message << endl;
            }
            sleep(1);
        }

        while (!replyQueue.empty())
        {
            string message = "1";
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
            string message = "0" + to_string(counter);
            mex.lock();
            pq.push(make_pair(counter, serverPort));
            mex.unlock();
            broadcast(endSockets, message);
            cout << "Sent REQUEST broadcast as message: " << message << endl;
            counter++;
        }

        sleep(1);
    }
}

void clientFn(int argc, char *argv[])
{
    // client
    if (argc < 3)
    {
        return;
    }
    vector<int> clientSockets;
    vector<int> ports;

    ports.resize(argc - 2);
    for (int i = 2; i < argc; i++)
    {
        ports[i - 2] = atoi(argv[i]);
        cout << ports[i - 2] << endl;
        port2ind[ports[i - 2]] = i - 2;
    }
    clientSockets.resize(ports.size());

    connectToPeers(clientSockets, ports);

    clientLoop(clientSockets, ports);

    for (auto &clientSocket : clientSockets)
    {
        close(clientSocket);
    }
}

bool bind(int &serverSocket, int port)
{
    int iOptionValue = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &iOptionValue, sizeof(int)) == -1)
    {
        printf("(Server Thread) ERROR setting socket options.\n");
        return false;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bzero((char *)&serverAddress, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        return false;
    }

    printf("(Server Thread) Server is listening to %i.\n", port);
    listen(serverSocket, 5);

    // Add the Non-Blocking param to the socket.

    return true;
}

void checkMessage(int ind, string &message)
{
    // 0 for REQ
    // 1 for REPLY
    // 2 for REL
    // 3 for REG
    int flag = (message[0] - '0');
    if (flag == 3)
    {
        int port = (stoi(message) % 10000);
        ind2Port[ind] = port;
    }
    else if (flag == 0)
    {
        // ind is requesting
        replyQueue.push(ind2Port[ind]);
        int timestamp = stoi(message.substr(1));
        mex.lock();
        pq.push(make_pair(timestamp, ind2Port[ind]));
        mex.unlock();
    }
    else if (flag == 1)
    {
        mex.lock();
        cout << "Got reply from " << ind2Port[ind] << " as message: " << message << endl;
        replyStatus[port2ind[ind2Port[ind]]] = true;
        mex.unlock();
    }
    else if (flag == 2)
    {
        removeFromPQ(ind2Port[ind]);
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
            printf("(Server Thread) select error\n");
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
            printf("(Server Thread) New connection , socket fd is %d , ip is : %s , port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

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
                    printf("(Server Thread) Host disconnected , ip %s , port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    endSockets[i] = 0;
                }
                else
                {
                    string message(buffer);
                    cout << "Message arrived as " << message << endl;
                    checkMessage(i, message);
                }
            }
        }
    }
}

void serverFn(int port)
{
    // server
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (!bind(serverSocket, port))
    {
        cout << "Error on binding to port" << endl;
    }

    // int clientSocket = accept(serverSocket, nullptr, nullptr);

    // fcntl(serverSocket, F_SETFL, fcntl(serverSocket, F_GETFL) | O_NONBLOCK);

    // char buffer[1024] = {0};
    // recv(clientSocket, buffer, sizeof(buffer), 0);
    // cout << "Message from client: " << clientSocket << buffer << endl;
    serve(serverSocket, port);

    close(serverSocket);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    maxPeer = argc - 2;
    serverPort = atoi(argv[1]);
    thread sender(clientFn, argc, argv);
    thread receiver(serverFn, serverPort);

    cout << "Start" << endl;
    sender.join();
    receiver.join();

    cout << "End";

    return 0;
}