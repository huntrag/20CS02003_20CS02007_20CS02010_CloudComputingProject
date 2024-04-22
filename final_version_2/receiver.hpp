#include "process.hpp"

class Receiver
{
private:
    Process *p;

protected:
    int receiverPort;

public:
    Receiver() {}
    Receiver(Process *p, int port)
    {
        this->p = p;
        this->receiverPort = port;
    }
    int getPort()
    {
        return receiverPort;
    }
    Process *getVar()
    {
        return this->p;
    }

    bool bindIt(int &receiverSocket, int port, string message);
    bool bindIt(int &receiverSocket, int port);
    void checkMessage(int ind, string &message);
    void serve(int master_socket, int port);
};

bool Receiver::bindIt(int &receiverSocket, int port, string ipaddress)
{
    int iOptionValue = 1;
    if (setsockopt(receiverSocket, SOL_SOCKET, SO_REUSEADDR, &iOptionValue, sizeof(int)) == -1)
    {
        printf("(receiver Thread) ERROR setting socket options.\n");
        return false;
    }

    sockaddr_in receiverAddress;
    bzero((char *)&receiverAddress, sizeof(receiverAddress));

    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.s_addr = inet_addr((const char *)ipaddress.c_str());
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

bool Receiver::bindIt(int &receiverSocket, int port)
{
    int iOptionValue = 1;
    if (setsockopt(receiverSocket, SOL_SOCKET, SO_REUSEADDR, &iOptionValue, sizeof(int)) == -1)
    {
        printf("(receiver Thread) ERROR setting socket options.\n");
        return false;
    }

    sockaddr_in receiverAddress;
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

void Receiver::checkMessage(int ind, string &message)
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
        // cout << "Got registration message as: " << message << endl;
        getVar()->ind2Port[ind] = port;
    }
    else if (flag == 0)
    {
        // ind is requesting
        int timestamp = stoi(message.substr(dollarPos + 1));
        int port = stoi(message.substr(hashPos + 1, dollarPos - hashPos - 1));
        // cout << "Got receive message from " << port << " as: " << message << endl;
        getVar()->replyQueue.push(port);
        getVar()->mex.lock();
        getVar()->pq.push(make_pair(timestamp, port));
        getVar()->mex.unlock();
    }
    else if (flag == 1)
    {
        getVar()->mex.lock();
        int port = stoi(message.substr(hashPos + 1));
        // cout << "Got reply from " << port << " as message: " << message << endl;
        getVar()->replyStatus[getVar()->port2ind[port]] = true;
        getVar()->mex.unlock();
    }
    else if (flag == 2)
    {
        int port = stoi(message.substr(hashPos + 1));
        getVar()->removeFromPQ(port);
    }
    else if (flag == 4)
    {
        getVar()->mex.lock();
        int port = stoi(message.substr(hashPos + 1));
        // cout << "Got start confirmation from " << port << " as message: " << message << endl;
        getVar()->startStatus[getVar()->port2ind[port]] = true;
        getVar()->mex.unlock();
    }
    else
    {
        cout << "Invalid message: " << message << endl;
    }
}

void Receiver::serve(int master_socket, int port)
{
    int i, sd, new_socket, addrlen, activity;
    int max_sd;
    fd_set readfds;
    struct sockaddr_in address;

    vector<int> endSockets(getVar()->maxPeer);

    while (1)
    {
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        // add child sockets to set
        for (i = 0; i < getVar()->maxPeer; i++)
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

            for (i = 0; i < getVar()->maxPeer; i++)
            {
                if (endSockets[i] == 0)
                {
                    endSockets[i] = new_socket;
                    break;
                }
            }
        }
        // main loop
        for (int i = 0; i < getVar()->maxPeer; i++)
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
                        // cout << "Received message: " << mess << endl;
                        message = message.substr(cur + 1);
                        checkMessage(i, mess);
                    }
                }
            }
        }
    }
}
