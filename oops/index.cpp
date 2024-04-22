#include "sender.hpp"

void senderFn(Sender *s, int argc, char *argv[])
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
        s->getVar()->port2ind[ports[i - 2]] = i - 2;
    }
    senderSockets.resize(ports.size());

    s->connectToPeers(senderSockets, ports);

    s->senderLoop(senderSockets, ports);

    for (auto &senderSocket : senderSockets)
    {
        close(senderSocket);
    }
}

void receiverFn(Receiver *r)
{
    // receiver
    int receiverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (!r->bindIt(receiverSocket, r->getPort()))
    {
        cout << "Error on binding to port" << endl;
    }

    // int senderSocket = accept(receiverSocket, nullptr, nullptr);

    // fcntl(receiverSocket, F_SETFL, fcntl(receiverSocket, F_GETFL) | O_NONBLOCK);

    // char buffer[1024] = {0};
    // recv(senderSocket, buffer, sizeof(buffer), 0);
    // cout << "Message from sender: " << senderSocket << buffer << endl;
    r->serve(receiverSocket, r->getPort());

    close(receiverSocket);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    Process *p = new Process(argc - 2);
    Receiver *r = new Receiver(p, atoi(argv[1]));
    Sender *s = new Sender(p, atoi(argv[1]));
    thread sender(senderFn, s, argc, argv);
    thread receiver(receiverFn, r);

    sender.join();
    receiver.join();

    return 0;
}