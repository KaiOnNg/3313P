// Include necessary headers
#include "socketserver.h"
#include "thread.h"
#include <iostream>
#include <vector>
#include <memory>

// Define a simple GameRoom class inheriting from Thread
class GameRoom : public Thread {
public:
    GameRoom() {}
    virtual long ThreadMain() {
        // Game logic goes here. For now, just a placeholder.
        std::cout << "Starting a new game room...\n";
        // Simulate game room activity
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::cout << "Game room ending...\n";
        return 0;
    }
};

int main() {
    Sync::SocketServer server(12345); // Listen on port 12345
    std::vector<std::unique_ptr<GameRoom>> gameRooms;

    std::cout << "Server started. Listening for connections...\n";

    // Accept connections and create game rooms
    try {
        while (true) {
            Sync::Socket newConnection = server.Accept();
            std::cout << "Accepted new connection. Starting a game room...\n";

            // Start a new game room for each connection
            gameRooms.push_back(std::make_unique<GameRoom>());
        }
    } catch (const std::string &e) {
        std::cerr << "Server exception: " << e << std::endl;
    }

    return 0;
}
