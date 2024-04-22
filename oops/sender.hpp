#include "receiver.hpp"

class Sender : public Receiver
{
private:
    Process *p;
    bool arrived;
    bool connected;
    bool registered;
    bool requested;
    int receiverPort;

public:
    Sender()
    {
        arrived = true;
        connected = false;
        registered = false;
        requested = false;
    }
    Sender(Process *p, int port)
    {
        this->p = p;
        arrived = true;
        connected = false;
        registered = false;
        requested = false;
        receiverPort = port;
    }
    Process *getVar()
    {
        return this->p;
    }
    struct socketAddr
    {
        int port;
        sockaddr_in receiverAddress;
        string ipaddress;

        socketAddr(int port)
        {
            bzero((char *)&receiverAddress, sizeof(receiverAddress));
            this->port = port;
            receiverAddress.sin_family = AF_INET;
            receiverAddress.sin_port = htons(port);
            receiverAddress.sin_addr.s_addr = INADDR_ANY;
        }

        socketAddr(int port, string ipaddress)
        {
            bzero((char *)&receiverAddress, sizeof(receiverAddress));
            this->port = port;
            receiverAddress.sin_family = AF_INET;
            receiverAddress.sin_port = htons(port);
            receiverAddress.sin_addr.s_addr = inet_addr((const char *)ipaddress.c_str());
            // receiverAddress.sin_addr.s_addr = inet_addr("192.168.1.100");
        }
    };
    void connectToPeers(vector<int> &senderSocket, vector<int> &ports);
    bool checkCritical(vector<int> &ports);
    void broadcast(vector<int> &endSockets, string &message);
    void sendMessage(int socket, string &message);
    bool checkStartStatus();
    void senderLoop(vector<int> &endSockets, vector<int> &ports);
};

void Sender::connectToPeers(vector<int> &senderSocket, vector<int> &ports)
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
    getVar()->replyStatus.resize(getVar()->maxPeer);
    getVar()->startStatus.resize(getVar()->maxPeer);
    cout << "Everybody connected " << connected << endl;
}

bool Sender::checkCritical(vector<int> &ports)
{
    getVar()->mex.lock();
    for (int i = 0; i < getVar()->maxPeer; i++)
    {
        if (!getVar()->replyStatus[i])
        {
            cout << "Reply from port pending with port: " << ports[i] << endl;
            getVar()->mex.unlock();
            return false;
        }
    }
    pair<int, int> cur = getVar()->pq.top();
    if (cur.second == receiverPort)
    {
        getVar()->cs = true;
    }
    getVar()->mex.unlock();
    return true;
}

void Sender::broadcast(vector<int> &endSockets, string &message)
{
    message.push_back('|');
    char buffer[message.length()];
    memset(buffer, '\0', message.length());
    strcpy(buffer, &message[0]);
    // cout << "Sent message (" << message << ") to broadcast" << endl;
    for (auto &endSocket : endSockets)
    {
        getVar()->mex.lock();
        send(endSocket, &buffer[0], message.length(), 0);
        getVar()->mex.unlock();
    }
}

void Sender::sendMessage(int socket, string &message)
{
    message.push_back('|');
    char buffer[message.length()];
    memset(buffer, '\0', message.length());
    strcpy(buffer, &message[0]);
    getVar()->mex.lock();
    send(socket, &buffer[0], message.length(), 0);
    getVar()->mex.unlock();
}

bool Sender::checkStartStatus()
{
    getVar()->mex.lock();
    for (int i = 0; i < getVar()->maxPeer; i++)
    {
        if (!getVar()->startStatus[i])
        {
            getVar()->mex.unlock();
            return false;
        }
    }
    getVar()->mex.unlock();
    return true;
}

void Sender::senderLoop(vector<int> &endSockets, vector<int> &ports)
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

        if (getVar()->ind2Port.size() < getVar()->maxPeer)
        {
            sleep(1);
            continue;
        }

        if (arrived)
        {
            cout << "Everybody's registrations arrived" << endl;
            for (auto &r : getVar()->ind2Port)
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
                getVar()->criticalSection();
                string message = "2#" + to_string(receiverPort);
                requested = false;
                getVar()->cs = false;
                fill(getVar()->replyStatus.begin(), getVar()->replyStatus.end(), false);
                broadcast(endSockets, message);
                cout << "Sent release broadcast as message: " << message << endl;
            }
        }

        while (!getVar()->replyQueue.empty())
        {
            string message = "1#" + to_string(receiverPort);
            sendMessage(endSockets[getVar()->port2ind[getVar()->replyQueue.front()]], message);
            cout << "Sent REPLY to port " << getVar()->replyQueue.front() << " as message: " << message << endl;
            getVar()->replyQueue.pop();
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
            string message = "0#" + to_string(receiverPort) + "$" + to_string(getVar()->timeStamp);
            getVar()->mex.lock();
            getVar()->pq.push(make_pair(getVar()->timeStamp, receiverPort));
            getVar()->mex.unlock();
            broadcast(endSockets, message);
            cout << "Sent REQUEST broadcast as message: " << message << endl;
            getVar()->incrementtimeStamp();
        }
        sleep(2);
    }
}