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
#include <stack>
#include <queue>
#include <map>

using std::cout;
using std::endl;
using std::greater;
using std::make_pair;
using std::map;
using std::mutex;
using std::pair;
using std::priority_queue;
using std::queue;
using std::stack;
using std::string;
using std::thread;
using std::to_string;
using std::vector;

// Global Variables
int maxPeer;
static int counter = 1;
bool connected = false;
vector<bool> replyStatus;
vector<bool> startStatus;
bool requested = false;
bool registered = false;
bool arrived = true;
bool cs = false;
int receiverPort;
priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
queue<int> replyQueue;
map<int, int> ind2Port;
map<int, int> port2ind;
mutex mex;

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

void criticalSection()
{
    cout << "Entered Section\n";
    sleep(3);
    cout << "Exit Section\n";
}

void removeFromPQ(int port)
{
    mex.lock();
    stack<pair<int, int>> stk;
    while (!pq.empty())
    {
        pair<int, int> cur = pq.top();
        pq.pop();
        if (cur.second == port)
        {
            break;
        }
        else
        {
            stk.push(cur);
        }
    }
    while (!stk.empty())
    {
        pq.push(stk.top());
        stk.pop();
    }
    mex.unlock();
}