// Include necessary headers
#include "socketserver.h"
#include "thread.h"
#include <iostream>
#include <vector>
#include <memory>
#include "Semaphore.h"
#include <algorithm>
#include <random>
#include "SharedObject.h"
#include <string>
#include <list>
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace Sync;
// set a global value
int count = 0;

// std::atomic<int> count(0);

std::vector<int> roomList = {3, 2, 1};

struct TableData
{
    int dealerHand[10];
    int playerHand[10];
    int dealerHandSize = 0; // New field to track number of cards in dealer's hand
    int playerHandSize = 0; // New field to track number of cards in player's hand
};

struct MyShared
{
    TableData table[3]; // Fixed-size array
};

class Dealer
{
private:
    std::vector<int> hand = std::vector<int>(10);

    // Additional functionality for dealer actions
public:
    Dealer() {}

    ~Dealer() {}

    // send card
    void deal(int (&dealerHand)[10])
    {
        std::copy(std::begin(dealerHand), std::end(dealerHand), this->hand.begin());
    }

    // used for calculayte the hand card
    int calculateHandTotal()
    {
        int total = 0;
        for (int card : this->hand)
        {
            total += card;
        }
        return total;
    }
};

class Player
{
private:
    std::vector<int> playerHand = std::vector<int>(10);
    std::vector<int> dealerHand = std::vector<int>(10);
    Socket socket;
    bool hitFlag = false;
    bool explode = false;
    bool isContinue = true;
    int roomId;
    Semaphore socketWrite;

    // Additional functionality for dealer actions
public:
    Player(Socket socket, int roomId) : socket(socket), roomId(roomId), socketWrite("socketWrite")
    {
        this->roomId = roomId;
        this->socket = socket;
    }

    ~Player() {}

    bool getHitFlag()
    {
        return hitFlag;
    }

    bool getExplode()
    {
        return explode;
    }

    void setExplode(bool explode)
    {
        this->explode = explode;
    }

    void setHitFlag(bool hit)
    {
        this->hitFlag = hit;
    }

    void askHit()
    {
        socketWrite.Wait();
        ByteArray data("Do you want to hit or stand?");
        socket.Write(data);

        ByteArray receivedData;
        socket.Read(receivedData);
        std::string ack(receivedData.v.begin(), receivedData.v.end());
        if (ack == "ack")
        {
            socketWrite.Signal();
        }

        // Wait for response
        while (true)
        {
            // Wait for response

            socket.Read(receivedData);
            std::string received(receivedData.v.begin(), receivedData.v.end());

            if (received == "hit")
            {
                hitFlag = true;
                break;
            }
            else if (received == "stand")
            {
                hitFlag = false;
                break;
            }
            else
            {
                socketWrite.Wait();
                ByteArray error("Invalid input, please try again.\n");
                socket.Write(error);
                socket.Read(receivedData);
                std::string ack(receivedData.v.begin(), receivedData.v.end());
                if (ack == "ack")
                {
                    socketWrite.Signal();
                }
            }
        }
    }

    bool getContinue()
    {

        return this->isContinue;
    }

    void readHand(int dealer[], int player[], int dealerHandSize, int playerHandSize)
    {
        // dealerHand.clear();
        // playerHand.clear();

        // playerHand will have the playerHand on shared memory
        std::copy_n(dealer, dealerHandSize, dealerHand.begin());
        std::copy_n(player, playerHandSize, playerHand.begin());

        std::string handStr;
        handStr += "Dealer's hand:\n";
        for (int i = 0; i < dealerHandSize; ++i)
        {
            handStr += std::to_string(dealerHand[i]) + " ";
        }
        handStr += "\nPlayer's hand:\n";
        for (int i = 0; i < playerHandSize; ++i)
        {
            handStr += std::to_string(playerHand[i]) + " ";
        }

        handStr += "\nYour total is: " + std::to_string(calculateHandTotal()) + "\n";

        socketWrite.Wait();
        ByteArray data(handStr);
        socket.Write(data);

        ByteArray receivedData;
        socket.Read(receivedData);
        std::string ack(receivedData.v.begin(), receivedData.v.end());
        if (ack == "ack")
        {
            socketWrite.Signal();
        }
    }

    void askContinue()
    {
        socketWrite.Wait();
        ByteArray data("Do you want to continue playing? (yes or no)");
        // message type = 1 hit
        socket.Write(data);
        ByteArray receivedData;
        socket.Read(receivedData);
        std::string ack(receivedData.v.begin(), receivedData.v.end());
        if (ack == "ack")
        {
            socketWrite.Signal();
        }

        socket.Read(data);
        std::string received(data.v.begin(), data.v.end());

        if (received == "yes")
        {
            isContinue = true;
        }
        else if (received == "no")
        {
            isContinue = false;
        }
    }

    void sendWinner(std::string string)
    {
        for (int i = 0; i < 10; i++)
        {
            playerHand[i] = 0;
            dealerHand[i] = 0;
        }
        socketWrite.Wait();
        socket.Write(string);
        ByteArray receivedData;
        socket.Read(receivedData);
        std::string ack(receivedData.v.begin(), receivedData.v.end());
        if (ack == "ack")
        {
            socketWrite.Signal();
        }
    }

    void closeConneton()
    {

        socket.Close();
    }

    // calculate the all hand card
    int calculateHandTotal()
    {
        int total = 0;
        for (int card : playerHand)
        {
            total += card;
        }
        return total;
    }
};

class Spectator
{
private:
    std::vector<int> playerHand = std::vector<int>(10);
    std::vector<int> dealerHand = std::vector<int>(10);
    Socket socket;
    int roomId;
    Semaphore socketWrite;
    // Additional functionality for dealer actions
public:
    Spectator(Socket socket) : socket(socket), socketWrite("socketWrite")
    {
        this->socket = socket;
    }

    ~Spectator() {}

    Socket getSocket()
    {
        return this->socket;
    }

    int getRoomId()
    {
        return this->roomId;
    }

    void send(int *dealer, int *player, int dealerHandSize, int playerHandSize)
    {
        std::copy_n(dealer, dealerHandSize, dealerHand.begin());
        std::copy_n(player, playerHandSize, playerHand.begin());
        std::string sendData = "Dealer's Hand:\n";
        for (int i = 0; i < dealerHandSize; i++)
        {
            sendData += std::to_string(dealerHand[i]) + " ";
        }
        sendData += "\nPlayer's Hand: \n";
        for (int i = 0; i < playerHandSize; i++)
        {
            sendData += std::to_string(playerHand[i]) + " ";
        }

        socketWrite.Wait();
        ByteArray data(sendData);
        socket.Write(data);
        ByteArray receivedData;
        socket.Read(receivedData);
        std::string ack(receivedData.v.begin(), receivedData.v.end());
        if (ack == "ack")
        {
            socketWrite.Signal();
        }
    }

    void setRoomId(int roomId)
    {
        this->roomId = roomId;
    }

    void askRoom()
    {
        // since when the room achieve 3, then it will assign the new comeer be the spectator
        socketWrite.Wait();
        ByteArray data("which rooom do you want to join in 1 or 2 or 3");
        socket.Write(data);
        ByteArray receivedData;
        socket.Read(receivedData);
        std::string ack(receivedData.v.begin(), receivedData.v.end());
        if (ack == "ack")
        {
            socketWrite.Signal();
        }
    }

    void sendWinner(std::string string)
    {
        socketWrite.Wait();
        socket.Write(string);
        ByteArray receivedData;
        socket.Read(receivedData);
        std::string ack(receivedData.v.begin(), receivedData.v.end());
        if (ack == "ack")
        {
            socketWrite.Signal();
        }
    }
};

// Define a simple GameRoom class inheriting from Thread
class GameRoom : public Thread
{
public:
    // main field
    int roomId;
    Dealer *gameDealer = new Dealer();
    Player *gamePlayer;
    bool continueFlag;
    std::vector<Spectator *> Spectatorlist;
    std::vector<std::vector<int>> deck;

    // create semaphore for read and write
    Semaphore *write;
    Semaphore *read;
    // semaphore arranging socket write
    Semaphore socketWrite;

    GameRoom(int roomId, Player *player, Semaphore *write, Semaphore *read) : Thread(), gamePlayer(player), roomId(roomId), write(write), read(read), socketWrite("socketWrite", 1, true)
    {
        this->gamePlayer = player;
        initializeDeck();
        this->roomId = roomId;
        this->write = write;
        this->read = read;
    }

    ~GameRoom() {}

    void addSpec(Spectator *newSpec)
    {
        Spectatorlist.push_back(newSpec);
    }

    // Initializes and shuffles the deck
    void initializeDeck()
    {
        deck.clear();
        deck.resize(4, std::vector<int>(13)); // Pre-fill each suit with space for 13 cards

        // Create a flat deck of 52 cards
        std::vector<int> flatDeck(52);
        int card = 0;
        // Fill the flat deck with card values
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 2; j <= 14; ++j, ++card)
            {                                        // Use 11 for Jack, 12 for Queen, 13 for King, 14 for Ace
                flatDeck[card] = (j <= 10) ? j : 10; // Face cards (Jack, Queen, King) are valued at 10, Ace initially as 11
                if (j == 14)
                    flatDeck[card] = 11; // Ace
            }
        }

        // Shuffle the flat deck
        std::shuffle(flatDeck.begin(), flatDeck.end(), std::default_random_engine(static_cast<unsigned int>(std::time(nullptr))));

        // Distribute the shuffled cards back into the suits
        for (int i = 0; i < 52; ++i)
        {
            deck[i % 4][i / 4] = flatDeck[i];
        }
    }

    void dealCard(std::vector<std::vector<int>> &deck, int hand[], int &handSize)
    {
        hand[handSize] = deck[0][deck[0].size() - 1];
        deck[0].pop_back();
        handSize++;
    }

    virtual long ThreadMain(void) override
    {
        try
        {
            try
            {
                continueFlag = true;
                while (continueFlag == true)
                {
                    Shared<MyShared> shared("sharedMemory");

                    std::cout << "Starting a new game..." << std::endl;
                    // initialize two card to the shared memory

                    this->initializeDeck();
                    gamePlayer->setExplode(false);
                    write->Wait();
                    for (int i = 0; i < 10; i++)
                    {
                        shared->table[roomId].dealerHand[i] = 0;
                        shared->table[roomId].playerHand[i] = 0;
                    }
                    shared->table[roomId].dealerHandSize = 0;
                    shared->table[roomId].playerHandSize = 0;
                    for (int i = 0; i < 2; ++i)
                    {
                        dealCard(deck, shared->table[roomId].dealerHand, shared->table[roomId].dealerHandSize);
                        dealCard(deck, shared->table[roomId].playerHand, shared->table[roomId].playerHandSize);
                    }
                    read->Signal();
                    read->Wait();
                    gameDealer->deal(shared->table[roomId].dealerHand);

                    if (Spectatorlist.size() != 0)
                    {
                        for (auto *spectator : Spectatorlist)
                        {
                            spectator->send(shared->table[roomId].dealerHand, shared->table[roomId].playerHand, shared->table[roomId].dealerHandSize, shared->table[roomId].playerHandSize);
                        }
                    }
                    gamePlayer->readHand(shared->table[roomId].dealerHand, shared->table[roomId].playerHand, shared->table[roomId].dealerHandSize, shared->table[roomId].playerHandSize);

                    write->Signal();
                    // player hit the card
                    while (true)
                    {
                        Shared<MyShared> shared("sharedMemory");
                        gamePlayer->askHit();

                        if (gamePlayer->getHitFlag() == true)
                        {
                            write->Wait();
                            dealCard(deck, shared->table[roomId].playerHand, shared->table[roomId].playerHandSize);
                            read->Signal();
                            
                            read->Wait();
                            if (Spectatorlist.size() != 0)
                            {
                                for (auto *spectator : Spectatorlist)
                                {
                                    spectator->send(shared->table[roomId].dealerHand, shared->table[roomId].playerHand, shared->table[roomId].dealerHandSize, shared->table[roomId].playerHandSize);
                                }
                            }

                            gamePlayer->readHand(shared->table[roomId].dealerHand, shared->table[roomId].playerHand, shared->table[roomId].dealerHandSize, shared->table[roomId].playerHandSize);

                            write->Signal();

                            if (gamePlayer->calculateHandTotal() > 21)
                            {
                                gamePlayer->setExplode(true);
                                break;
                            };
                        }
                        else if (gamePlayer->getHitFlag() == false)
                        {
                            break;
                        };
                    }

                    // dealer begin hit the card
                    while (true)
                    {
                        if ((!gamePlayer->getExplode() && gameDealer->calculateHandTotal() < 17))
                        {
                            // if dealer satisfy the condition then add card to shared memory
                            // shared->dealerHand1.push_back(deck[rand() % 4][rand() % 13]);
                            Shared<MyShared> shared("sharedMemory");
                            write->Wait();
                            // shared->table[0].dealerHand[shared->table[0].dealerHandSize] = 3; // Add a card (e.g., '3')
                            // shared->table[0].dealerHandSize++;                                // Increment the count
                            dealCard(deck, shared->table[roomId].dealerHand, shared->table[roomId].dealerHandSize);
                            read->Signal();

                            read->Wait();
                            gameDealer->deal(shared->table[roomId].dealerHand);

                            // let spector get information
                            if (Spectatorlist.size() != 0)
                            {
                                for (auto *spectator : Spectatorlist)
                                {
                                    spectator->send(shared->table[roomId].dealerHand, shared->table[roomId].playerHand, shared->table[roomId].dealerHandSize, shared->table[roomId].playerHandSize);
                                }
                            }
                            // since the player has finished his round and act as a spectator
                            gamePlayer->readHand(shared->table[roomId].dealerHand, shared->table[roomId].playerHand, shared->table[roomId].dealerHandSize, shared->table[roomId].playerHandSize);
                            write->Signal();
                        }
                        else
                        {
                            break;
                        }
                    }
                    // if player exploded
                    if (gamePlayer->getExplode())
                    {
                        // dealer wins, notify player and all spectators
                        gamePlayer->sendWinner("Dealer wins!!");
                        if (Spectatorlist.size() != 0)
                        {
                            for (auto *spectator : Spectatorlist)
                            {
                                spectator->sendWinner("Dealer wins!!\n");
                            }
                        }
                    }
                    // if dealer exploded
                    else if (gameDealer->calculateHandTotal() > 21)
                    {
                        // Player wins, notify player and all spectators
                        gamePlayer->sendWinner("Player wins!!\n");
                        if (Spectatorlist.size() != 0)
                        {
                            for (auto *spectator : Spectatorlist)
                            {
                                spectator->sendWinner("Player wins!!\n");
                            }
                        }
                    }

                    // compare score
                    else if (gameDealer->calculateHandTotal() >= gamePlayer->calculateHandTotal())
                    {
                        // dealer wins, notify player and all spectators
                        gamePlayer->sendWinner("Dealer wins!!\n");
                        if (Spectatorlist.size() != 0)
                        {
                            for (auto *spectator : Spectatorlist)
                            {
                                spectator->sendWinner("Dealer wins!!\n");
                            }
                        }
                    }
                    else
                    {
                        // Player wins, notify player and all spectators
                        gamePlayer->sendWinner("Player wins!!\n");
                        if (Spectatorlist.size() != 0)
                        {
                            for (auto *spectator : Spectatorlist)
                            {
                                spectator->sendWinner("Player wins!!\n");
                            }
                        }
                    }

                    gamePlayer->askContinue();
                    if (gamePlayer->getContinue())
                    {
                        // nothing change
                        // Write something to client
                        gamePlayer->sendWinner("You continue playing!\n");
                    }
                    else
                    {
                        // let the connetion close
                        gamePlayer->sendWinner("Bye\n");
                        // Check if the spectator list is not empty to avoid undefined behavior
                        if (!Spectatorlist.empty())
                        { // Accessing the first spectator
                            Spectator *firstSpectator = Spectatorlist.front();
                            // Or Spectatorlist[0] // Then set the first spectator as the player // Assign the member to here
                            gamePlayer = new Player(firstSpectator->getSocket(), firstSpectator->getRoomId()); // Reset the first place of vector list by Removing the first element
                            Spectatorlist.erase(Spectatorlist.begin());
                        }
                        else
                        {
                            gamePlayer = nullptr;
                        }
                    }
                    if (gamePlayer == nullptr && Spectatorlist.size() == 0)
                    {
                        // terminate the thread if there is no player and the list is empty
                        continueFlag = false;
                    }
                    else
                    {
                        continueFlag == true;
                    }
                }
                std::this_thread::sleep_for(std::chrono::seconds(2));
                std::cout << "Game room ending...\n";

                roomList.push_back(roomId);
                count--;
            }
            catch (std::string &e)
            {
                // system output
                std::cout << "cannot create server" << std::endl;
            }
        }
        catch (std::string &e)
        {
            // system output
            std::cout << e << std::endl;
        }
        return 0;
    };
};

int main()
{
    SocketServer server(12345); // Listen on port 12345
    std::vector<GameRoom *> gameRooms;
    std::cout << "Server started. Listening for connections...\n";
    Semaphore write = Semaphore("write", 1, true);
    Semaphore read = Semaphore("read", 0, true);
    Shared<MyShared> sharedMemory("sharedMemory", true);
    // Accept connections and create game rooms
    try
    {
        while (true)
        {
            Socket newConnection = server.Accept();

            std::cout << "Accepted new connection.\n";

            if (count < 3)
            {
                // Start a new game room for each connection
                std::cout << "Starting a game room...\n";

                int roomID = roomList[roomList.size() - 1];

                gameRooms.push_back(new GameRoom(roomID, new Player(newConnection, roomID), &write,&read ));
                count++;

                roomList.pop_back();
            }
            else
            {
                // send message to client tell him the room is room full
                // which room you want to join
                // wait response
                //  if (out of range ---- retry) inside the range put to the spectator list
                // send current situation of current rooom
                std::cout << "Join as a spectator\n";
                Spectator *newSpec = new Spectator(newConnection);
                newSpec->askRoom();
                ByteArray response;
                newConnection.Read(response);
                std::string receive(response.v.begin(), response.v.end());
                if (receive.length() == 0)
                {
                    break;
                }
                if (receive == "1")
                {
                    // newSpec->setSemaphore(read);
                    newSpec->setRoomId(1);
                    gameRooms[0]->addSpec(newSpec);
                }
                else if (receive == "2")
                {
                    // newSpec->setSemaphore(read);
                    newSpec->setRoomId(2);
                    gameRooms[1]->addSpec(newSpec);
                }
                else if (receive == "3")
                {
                    // in room 3
                    // newSpec->setSemaphore(read);
                    newSpec->setRoomId(3);
                    gameRooms[2]->addSpec(newSpec);
                }
                else
                {
                    break;
                }
            }
        }
    }
    catch (const std::string &e)
    {
        std::cerr << "Server exception: " << e << std::endl;
        server.Shutdown();
    }

    return 0;
}