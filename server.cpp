// Include necessary headers
#include "socketserver.h"
#include "thread.h"
#include <iostream>
#include <vector>
#include <memory>
#include "semaphore.h"
#include <algorithm>
#include <random>

// Define a simple GameRoom class inheriting from Thread
class GameRoom : public Thread {
private:
    std::vector<int> deck;

     // Initializes and shuffles the deck
    void initializeDeck() {
        deck.clear(); // Ensure the deck starts empty

        // Add four of each card value, excluding face cards which are all valued at 10
        for (int i = 0; i < 4; ++i) { // Four suits
            for (int j = 2; j <= 10; ++j) { // Cards 2-10
                deck.push_back(j);
            }

            // Face cards and Aces
            deck.insert(deck.end(), {10, 10, 10, 10, 11}); // J, Q, K, A (Aces are initially 11)
        }

        // Shuffle the deck
        auto rng = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
        std::shuffle(deck.begin(), deck.end(), rng);
    }

public:
    GameRoom() {
        initializeDeck();
    }
    virtual long ThreadMain() override{
        // Game logic goes here. For now, just a placeholder.
        std::cout << "Starting a new game room...\n";
        // Simulate game room activity
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::cout << "Game room ending...\n";
        return 0;
    }
};

class Player : {
private: 
    std::vector<int> playerHand;
    
public:
};

class Dealer : {
private:
    std::vector<int> dealerHand;
};

class Spectator : {
}

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
