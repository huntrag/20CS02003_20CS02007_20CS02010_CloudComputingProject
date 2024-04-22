#include <bits/stdc++.h>
using namespace std;

int main()
{
    string message = "3#8082|4#8082|asdas|as|";
    int cur = 0;
    while ((cur = message.find_first_of('|')) < message.length())
    {
        string mess = message.substr(0, cur);
        message = message.substr(cur + 1);
        cout << mess << endl;
    }
    return 0;
}