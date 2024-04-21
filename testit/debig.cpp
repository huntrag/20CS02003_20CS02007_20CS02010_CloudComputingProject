#include <thread>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mutex>
#include <vector>
#include <queue>

using namespace std;

struct connStruct
{
    int port;
    sockaddr_in serverAddress;

    connStruct(int port)
    {
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = INADDR_ANY;
    }
};

int createSocket(int port)
{
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
}

void connectToPeers(vector<int> &clientSocket, vector<int> &ports)
{
    auto port = ports.begin();
    vector<connStruct *> connArr(ports.size());
    queue<int> connectSockets;
    int i = 0;
    for (auto &socket : clientSocket)
    {
        connArr[i] = new connStruct(*port);
        socket = createSocket(*port);
        connectSockets.push(i++);
        port++;
    }

    while (!connectSockets.empty())
    {
        int ind = connectSockets.front();
        connectSockets.pop();
        if (connect(clientSocket[ind], (struct sockaddr *)&connArr[ind]->serverAddress, sizeof(connArr[ind]->serverAddress)))
        {
            cout << "Connected to " << connArr[i]->port << endl;
        }
        else
        {
            cout << "No response from " << connArr[i]->port << endl;
            connectSockets.push(ind);
        }
        sleep(1);
    }
}

void sendThrFn(vector<int> &ports)
{
    // client
    vector<int> clientSockets(ports.size());

    connectToPeers(clientSockets, ports);

    string message = "Hello, server!";
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

void recThrFn(int port)
{
    // server
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

    if (!~listen(serverSocket, 5))
        cout << "Cannot listen.";

    int clientSocket = accept(serverSocket, nullptr, nullptr);

    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);
    cout << "Message from client: " << buffer << endl;

    close(serverSocket);
}

int main(int argc, char *argv[])
{
    int sport = atoi(argv[1]);
    vector<int> cports(argc - 2);
    for (int i = 2; i < argc; i++)
    {
        cports[i - 2] = atoi(argv[i]);
    }

    thread sender(sendThrFn, cports);
    thread receiver(recThrFn, sport);

    cout << "Start" << endl;
    sender.join();
    receiver.join();
    cout << "End";

    return 0;
}