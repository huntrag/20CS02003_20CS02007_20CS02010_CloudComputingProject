#include <bits/stdc++.h>
using namespace std;

void updateFile(const std::string &filename)
{
    std::ifstream input(filename);
    if (!input.is_open())
    {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return;
    }

    std::string firstLine;
    if (std::getline(input, firstLine))
    {
        input.close();

        int number = std::stoi(firstLine) + 2;         // Add 2 to the number obtained from the first line
        std::ofstream output(filename, std::ios::app); // Open file in append mode
        if (!output.is_open())
        {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return;
        }
        output << "hello world" << std::endl; // Append "hello world" to the end of the file
        output.close();

        std::fstream overwrite(filename, std::ios::in | std::ios::out); // Open file in read/write mode
        if (!overwrite.is_open())
        {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return;
        }
        overwrite.seekp(0);               // Move write pointer to the beginning of the file
        overwrite << number << std::endl; // Write the updated number to the first line
    }
    else
    {
        std::cerr << "File is empty: " << filename << std::endl;
    }
}
class F1
{
protected:
    int x;
    int y;

public:
    int getX()
    {
        return x;
    }
    int getY()
    {
        return y;
    }
};

class F2 : public F1
{
private:
    int f;

public:
    void getF()
    {
    }
};
std::string getCurrentTime()
{
    // Get the current time point
    auto now = std::chrono::system_clock::now();

    // Extract the time components
    auto time_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // Convert to local time
    std::tm *local_time = std::localtime(&time_now);

    // Format the time components as a string
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << local_time->tm_hour << ":";
    ss << std::setw(2) << std::setfill('0') << local_time->tm_min << ":";
    ss << std::setw(2) << std::setfill('0') << local_time->tm_sec << ".";
    ss << std::setw(3) << std::setfill('0') << ms.count();

    return ss.str();
}

int main()
{
    // Get the current time point
    auto now = std::chrono::system_clock::now();

    // Extract the time components
    auto time_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // Convert to local time
    std::tm *local_time = std::localtime(&time_now);

    // Print the time components
    std::cout << "Time: ";
    std::cout << local_time->tm_hour << ":" << local_time->tm_min << ":" << local_time->tm_sec << "." << ms.count() << std::endl;

    cout << getCurrentTime();
    return 0;
}