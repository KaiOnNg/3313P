#include <iostream>
#include <string>
#include <sstream>
#include <cstring>      // For memset
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <unistd.h>     // For read/write/close

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(12345); // Server port number
    //servaddr.sin_addr.s_addr = inet_addr("127.0.0.1",12345); // Server IP, localhost for testing

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "Connection to the server failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected to the server." << std::endl;

    std::string input;
    char buffer[1024] = {0};
    while (true) {
        // Wait for response
        int n = read(sockfd, buffer, sizeof(buffer));
        if (n > 0) {
            if (std::string(buffer, n) == "bye"){
                break;
            }
            std::cout << "Server: " << std::string(buffer, n) << std::endl;
        } else {
            std::cout << "No response from server, or connection closed." << std::endl;
            break;
        }


        std::cout << "> ";
        std::getline(std::cin, input); // Get input from the user

        // Send input to the server
        send(sockfd, input.c_str(), input.length(), 0);

        memset(buffer, 0, sizeof(buffer)); // Clear buffer
    }

    // Close the socket
    close(sockfd);

    return 0;
}
