#include "sender.hpp"

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