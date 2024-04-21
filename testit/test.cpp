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
            cout << "Connected to " << connArr[ind]->port << endl;
        }
        else
        {
            cout << "No response from " << connArr[ind]->port << endl;
            connectSockets.push(ind);
        }
        sleep(1);
    }
}

void sendThrFn(int argc, char *argv[])
{
    // client
    if (argc < 3)
    {
        return;
    }
    vector<int> clientSockets;
    vector<int> ports;
    try
    {
        ports.resize(argc - 2);
        for (int i = 2; i < argc; i++)
        {
            ports[i - 2] = atoi(argv[i]);
            cout << ports[i - 2] << endl;
        }
        clientSockets.resize(ports.size());
    }
    catch (...)
    {
        cout << "Error in port inits" << endl;
    }

    try
    {
        connectToPeers(clientSockets, ports);
    }
    catch (...)
    {
        cout << "Error in connecting" << endl;
    }

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
    int sport;
    try
    {
        sport = atoi(argv[1]);
    }
    catch (...)
    {
        cout << "Error at the beginning" << endl;
    }
    cout << sport << endl;
    thread sender(sendThrFn, argc, argv);
    thread receiver(recThrFn, sport);

    cout << "Start" << endl;
    try
    {
        sender.join();
        receiver.join();
    }
    catch (...)
    {
        cout << "Dont know" << endl;
    }
    cout << "End";

    return 0;
}