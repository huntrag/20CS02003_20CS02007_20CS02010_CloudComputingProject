#include "receiver.hpp"

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