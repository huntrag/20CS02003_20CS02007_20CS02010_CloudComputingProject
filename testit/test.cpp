#include <thread>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mutex>

using namespace std;

mutex mut;
void sendThrFn(int port)
{
    // client
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    while (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)))
    {
        sleep(1);
        cout << "Waiting for " << port << endl;
    }

    string message = "Hello, server!";
    char buffer[message.size()];
    strcpy(buffer, &message[0]);
    send(clientSocket, buffer, message.length(), 0);

    close(clientSocket);
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
    int cport = atoi(argv[1]);
    int sport = atoi(argv[2]);

    thread sender(sendThrFn, cport);
    thread receiver(recThrFn, sport);

    cout << "Start" << endl;
    sender.join();
    receiver.join();
    cout << "End";

    return 0;
}