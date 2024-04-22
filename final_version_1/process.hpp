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
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

using std::cout;
using std::endl;
using std::fstream;
using std::greater;
using std::ifstream;
using std::make_pair;
using std::map;
using std::mutex;
using std::ofstream;
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
    int receiverPort;
    string filename;

    Process() {}

    Process(int maxPeer, int receiverPort, string filename)
    {
        this->maxPeer = maxPeer;
        this->filename = filename;
        this->receiverPort = receiverPort;
        ofstream file(filename);
        if (file.is_open())
        {
            file << "0";
            file.close();
        }
        timeStamp = 1;
    }

    void incrementtimeStamp()
    {
        timeStamp = timeStamp + 1;
    }

    string getCurrentTime()
    {
        auto now = std::chrono::system_clock::now();
        auto time_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::tm *local_time = std::localtime(&time_now);
        std::stringstream ss;
        ss << "(";
        ss << std::setw(2) << std::setfill('0') << local_time->tm_hour << ":";
        ss << std::setw(2) << std::setfill('0') << local_time->tm_min << ":";
        ss << std::setw(2) << std::setfill('0') << local_time->tm_sec << ".";
        ss << std::setw(3) << std::setfill('0') << ms.count();
        ss << ") ";
        return ss.str();
    }

    void updateFile()
    {
        ifstream input(filename);
        if (!input.is_open())
        {
            cout << "Failed to open file for reading: " << filename << endl;
            return;
        }
        string firstLine;
        if (std::getline(input, firstLine))
        {
            input.close();
            int randomNumber = rand() % 10;
            int number = stoi(firstLine);
            ofstream output(filename, std::ios::app);
            output << getCurrentTime() << "Port " << receiverPort << " changed first line entering CS by adding " << randomNumber << " to " << number << " getting " << randomNumber + number << endl;
            output.close();
            fstream overwrite(filename, std::ios::in | std::ios::out);
            overwrite.seekp(0);
            overwrite << number + randomNumber << endl;
        }
        else
        {
            cout << "File is empty: " << filename << endl;
        }
    }

    void criticalSection()
    {
        cout << getCurrentTime() << "Entered CS. Modifying file contents" << endl;
        updateFile();
        sleep(3);
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