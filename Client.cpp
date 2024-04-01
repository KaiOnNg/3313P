#include <iostream>
#include <string>
#include <algorithm>
#include "Semaphore.h"
#include <sstream>
#include <cstring>      // For memset
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <unistd.h>     // For read/write/close
std::string trim(const std::string &str)
{
    auto start = std::find_if(str.begin(), str.end(), [](unsigned char ch)
                              { return !std::isspace(ch); });

    auto end = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch)
                            { return !std::isspace(ch); })
                   .base();

    // Check if the string is all whitespace
    if (start >= end)
        return "";

    return std::string(start, end);
}

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;
    Semaphore socketWrite = Semaphore("socketWrite");

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "Socket creation failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(12345); // Server port number
    // servaddr.sin_addr.s_addr = inet_addr("127.0.0.1",12345); // Server IP, localhost for testing

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cerr << "Connection to the server failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected to the server." << std::endl;

    std::string input;
    char buffer[1024] = {0};
    while (true)
    {
        memset(buffer, 0, sizeof(buffer)); // Clear buffer before reading
        int n = read(sockfd, buffer, sizeof(buffer));
        socketWrite.Signal();

        if (n > 0)
        {
            buffer[n] = '\0';
            std::string receivedMsg(buffer, n);

            if (receivedMsg == "bye")
            {
                std::cout << std::endl;
                break;
            }
            // trim the receivedMsg and remove any trailing whitespace
            // Trim and remove null characters
            std::string trimMsg = trim(receivedMsg);

            std::cout << trimMsg << std::endl;

            // React to specific server prompts
            if (trimMsg == "Do you want to hit or stand?" || trimMsg == "Do you want to continue playing? (yes or no)" || trimMsg == "which rooom do you want to join in 1 or 2 or 3" || trimMsg == "Invalid input, please try again.")
            {
                std::cout << "> ";
                std::getline(std::cin, input); // Get input from the user
                                               // Send input to the server with error checking
                ssize_t bytesSent = send(sockfd, input.c_str(), input.length(), 0);
                if (bytesSent < 0)
                {
                    std::cerr << "Failed to send data to server." << std::endl;
                    break; // or handle error as appropriate
                }
            }
        }
        else
        {
            std::cout << "No response from server, or connection closed." << std::endl;
            break;
        }
    }

    // Close the socket
    close(sockfd);

    return 0;
}
