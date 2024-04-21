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
#include <queue>

// Global Variables
int maxPeer;

using namespace std;

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

class Socket
{
private:
    int serverSocket;
    int serverPort;
    vector<int> clientSockets;
    vector<int> clientPorts;

public:
    void connectToPeers();
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
    }
    clientSockets.resize(ports.size());

    connectToPeers(clientSockets, ports);

    string message = "(Client Thread) Hello, server!";
    char buffer[message.size()];
    strcpy(buffer, &message[0]);
    for (auto &clientSocket : clientSockets)
    {
        send(clientSocket, buffer, message.length(), 0);
    }

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

void serve(int master_socket, int port)
{
    int i, sd, new_socket, addrlen, activity;
    int max_sd;
    fd_set readfds;
    struct sockaddr_in address;
    char buffer[1025];
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
            printf("(Server Thread) select error");
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
            printf("(Server Thread) New connection , socket fd is %d , ip is : %s , port : %d  ", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

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
            sd = endSockets[i];
            if (FD_ISSET(sd, &readfds))
            {
                int valread;
                if ((valread = read(sd, buffer, 1024)) == 0)
                {
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("(Server Thread) Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    endSockets[i] = 0;
                }
                else
                {
                    string message(buffer);
                    cout << message << endl;
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
    maxPeer = argc - 2;
    int sport = atoi(argv[1]);
    cout << sport << endl;
    thread sender(clientFn, argc, argv);
    thread receiver(serverFn, sport);

    cout << "Start" << endl;
    sender.join();
    receiver.join();

    cout << "End";

    return 0;
}