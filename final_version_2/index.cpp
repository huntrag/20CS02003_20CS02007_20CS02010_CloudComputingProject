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
    vector<string> ips;

    ports.resize((argc - 1) / 2 - 1);
    ips.resize(ports.size());
    for (int i = 3; i < argc; i += 2)
    {
        int ind = (i - 3) / 2;
        ports[ind] = atoi(argv[i]);
        cout << ports[ind] << endl;
        s->getVar()->port2ind[ports[ind]] = ind;
    }

    for (int i = 4; i < argc; i += 2)
    {
        int ind = (i - 4) / 2;
        ips[ind] = argv[i];
        cout << ips[ind] << endl;
    }

    senderSockets.resize(ports.size());

    s->connectToPeers(senderSockets, ports, ips);

    s->senderLoop(senderSockets, ports);

    for (auto &senderSocket : senderSockets)
    {
        close(senderSocket);
    }
}

void receiverFn(Receiver *r, string ip)
{
    // receiver
    int receiverSocket = socket(AF_INET, SOCK_STREAM, 0);
    cout << "Server " << ip << endl;
    if (!r->bindIt(receiverSocket, r->getPort(), ip))
    {
        cout << "Error on binding to port" << endl;
    }
    r->serve(receiverSocket, r->getPort());
    close(receiverSocket);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    string serverIp(argv[2]);
    Process *p = new Process(argc - 2, atoi(argv[1]), "test.txt");
    Receiver *r = new Receiver(p, atoi(argv[1]));
    Sender *s = new Sender(p, atoi(argv[1]));
    thread sender(senderFn, s, argc, argv);
    thread receiver(receiverFn, r, serverIp);

    sender.join();
    receiver.join();

    return 0;
}