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
class Process
{
public:
    int maxPeer;
    int timeStamp;
    vector<bool> replyStatus;
    vector<bool> startStatus;
    bool cs = false;
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    queue<int> replyQueue;
    map<int, int> ind2Port;
    map<int, int> port2ind;
    mutex mex;

    Process() {}

    Process(int maxPeer)
    {
        this->maxPeer = maxPeer;
        timeStamp = 1;
    }

    void incrementtimeStamp()
    {
        timeStamp = timeStamp + 1;
    }

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
};