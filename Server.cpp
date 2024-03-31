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
    Semaphore write;
    Semaphore read;

    // Additional functionality for dealer actions
public:
    Player(Socket socket, int roomId, Semaphore write, Semaphore read) : socket(socket), roomId(roomId), write(write), read(read)
    {
        this->roomId = roomId;
        this->socket = socket;
        this->write = write;
        this->read = read;
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

    void askHit()
    {

        // Wait for response
        while (true)
        {
            ByteArray data("Do you want to hit or stand");
            socket.Write(data);

            // Wait for response

            ByteArray receivedData;
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
                ByteArray error("Invalid input, please try again.");
                socket.Write(error);
            }
        }
    }

    bool getContinue()
    {

        return this->isContinue;
    }

    void readHand(Shared<MyShared> sharedmemory)
    {

        write.Wait();
        // playerHand will have the playerHand on shared memory
        playerHand.clear();
        dealerHand.clear();
        std::copy_n(sharedmemory->table[roomId].playerHand, sharedmemory->table[roomId].playerHandSize, std::back_inserter(playerHand));
        std::copy_n(sharedmemory->table[roomId].dealerHand, sharedmemory->table[roomId].dealerHandSize, std::back_inserter(dealerHand));
        // have not define the read and wirte semaphore
        write.Signal();
        // set up the string for printing
        std::string handStr;
        handStr += "Dealer's hand: ";
        for (int i = 0; i < dealerHand.size(); ++i)
        {
            handStr += std::to_string(dealerHand[i]) + " ";
        }

        handStr += "\nPlayer's hand: ";
        for (int i = 0; i < playerHand.size(); ++i)
        {
            handStr += std::to_string(playerHand[i]) + " ";
        }

        ByteArray data(handStr);
        socket.Write(data);
        // have not define the read and wirte semaphore
        write.Signal();
    }

    void askContinue()
    {
        ByteArray data("Do you want to continue playing? (yes or no)");
        // message type = 1 hit
        socket.Write(data);

        int receivedData = socket.Read(data);
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

        socket.Write(string);
    }

    void closeConneton()
    {

        socket.Close();
    }

    // calculate the all hand card
    int calculateHandTotal()
    {
        int total = 0;
        for (int card : this->playerHand)
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
    Semaphore read;
    // Additional functionality for dealer actions
public:
    Spectator(Socket socket, Semaphore read) : socket(socket), read(read)
    {
        this->socket = socket;
        this->read = read;
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

    void setSemaphore(Semaphore read)
    {
        this->read = read;
    }

    void send(Shared<MyShared> sharedMemory)
    {

        read.Wait();

        read.Signal();
        // avoid dead lock
        std::string sendData = "Player: ";
        for (int card : playerHand)
        {
            sendData += std::to_string(card) + " ";
        }
        sendData += "Dealer: ";
        for (int card : dealerHand)
        {
            sendData += std::to_string(card) + " ";
        }

        ByteArray data(sendData);
        socket.Write(data);
    }

    void setRoomId(int roomId)
    {
        this->roomId = roomId;
    }

    void askRoom()
    {
        // since when the room achieve 3, then it will assign the new comeer be the spectator
        ByteArray data("which rooom do you want to join in 1 or 2 or 3");
        socket.Write(data);
    }

    void sendWinner(std::string string)
    {
        socket.Write(string);
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
    Semaphore write;
    Semaphore read;
    //

    GameRoom(int roomId, Player *player, Semaphore write, Semaphore read) : Thread(), gamePlayer(player), roomId(roomId), write(write), read(read)
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
            // Shared<MyShared> shared("sharedMemory");

            try
            {
                continueFlag = true;
                while (continueFlag == true)
                {
                    Shared<MyShared> shared("sharedMemory");
                    std::cout << "Starting a new game..." << std::endl;
                    // initialize two card to the shared memory

                    write.Wait();
                    this->initializeDeck();
                    for (int i = 0; i < 10; i++)
                    {
                        shared->table[roomId].dealerHand[i] = 0;
                        shared->table[roomId].playerHand[i] = 0;
                    }
                    shared->table[roomId].dealerHandSize = 0;
                    shared->table[roomId].playerHandSize = 0;
                    write.Signal();

                    for (int i = 0; i < 2; ++i)
                    {
                        dealCard(deck, shared->table[roomId].dealerHand, shared->table[roomId].dealerHandSize);
                        dealCard(deck, shared->table[roomId].playerHand, shared->table[roomId].playerHandSize);
                    }

                    write.Wait();
                    gameDealer->deal(shared->table[roomId].dealerHand);
                    write.Signal();

                    if (Spectatorlist.size() != 0)
                    {
                        for (auto *spectator : Spectatorlist)
                        {
                            spectator->send(shared);
                        }
                    }

                    gamePlayer->readHand(shared);

                    // player hit the card
                    while (true)
                    {
                        Shared<MyShared> shared("sharedMemory");
                        gamePlayer->askHit();

                        if (gamePlayer->getHitFlag())
                        {
                            std::cout << "hit" << std::endl;
                            write.Wait();
                            // shared->table[0].playerHand[shared->table[0].playerHandSize] = 3; // Add a card (e.g., '3')
                            // shared->table[0].playerHandSize++;                                // Increment the count
                            dealCard(deck, shared->table[roomId].playerHand, shared->table[roomId].playerHandSize);
                            std::cout << "New playerhand size: " << shared->table[roomId].playerHandSize << std::endl;
                            write.Signal();
                            gamePlayer->readHand(shared);
                        }
                        else if (gamePlayer->getHitFlag() == false)
                        {
                            break;
                        };

                        if (gamePlayer->calculateHandTotal() > 21)
                        {
                            gamePlayer->setExplode(true);
                            break;
                        };

                        if (Spectatorlist.size() != 0)
                        {
                            for (auto *spectator : Spectatorlist)
                            {
                                spectator->send(shared);
                            }
                        }
                    }

                    // dealer begin hit the card

                    while ((!gamePlayer->getExplode() && gameDealer->calculateHandTotal() < 17))
                    {
                        // if dealer satisfy the condition then add card to shared memory
                        // shared->dealerHand1.push_back(deck[rand() % 4][rand() % 13]);
                        Shared<MyShared> shared("sharedMemory");
                        write.Wait();
                        // shared->table[0].dealerHand[shared->table[0].dealerHandSize] = 3; // Add a card (e.g., '3')
                        // shared->table[0].dealerHandSize++;                                // Increment the count
                        dealCard(deck, shared->table[roomId].dealerHand, shared->table[roomId].dealerHandSize);
                        std::cout << "New dealerhand size: " << shared->table[roomId].dealerHandSize << std::endl;
                        gameDealer->deal(shared->table[roomId].dealerHand);
                        write.Signal();

                        // let spector get information
                        if (Spectatorlist.size() != 0)
                        {
                            for (auto *spectator : Spectatorlist)
                            {
                                spectator->send(shared);
                            }
                        }
                        // since the player has finished his round and act as a spectator
                        gamePlayer->readHand(shared);
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
                                spectator->sendWinner("Dealer wins!!");
                            }
                        }
                    }

                    // if dealer exploded
                    if (gameDealer->calculateHandTotal() > 21)
                    {
                        // Player wins, notify player and all spectators
                        gamePlayer->sendWinner("Player wins!!");
                        if (Spectatorlist.size() != 0)
                        {
                            for (auto *spectator : Spectatorlist)
                            {
                                spectator->sendWinner("Player wins!!");
                            }
                        }
                    }

                    // compare score
                    if (gameDealer->calculateHandTotal() > gamePlayer->calculateHandTotal())
                    {
                        // dealer wins, notify player and all spectators
                        gamePlayer->sendWinner("Dealer wins!!");
                        if (Spectatorlist.size() != 0)
                        {
                            for (auto *spectator : Spectatorlist)
                            {
                                spectator->sendWinner("Dealer wins!!");
                            }
                        }
                    }
                    else
                    {
                        // Player wins, notify player and all spectators
                        gamePlayer->sendWinner("Player wins!!");
                        if (Spectatorlist.size() != 0)
                        {
                            for (auto *spectator : Spectatorlist)
                            {
                                spectator->sendWinner("Player wins!!");
                            }
                        }
                    }

                    gamePlayer->askContinue();
                    if (gamePlayer->getContinue())
                    {
                        // nothing change
                        // Write something to client
                        gamePlayer->sendWinner("You continue playing!");
                    }
                    else
                    {
                        // let the connetion close
                        gamePlayer->sendWinner("Bye");
                        gamePlayer->closeConneton();
                        // then set the first spectator as the player
                        // assign the member to here
                        gamePlayer = new Player(Spectatorlist[0]->getSocket(), Spectatorlist[0]->getRoomId(), this->write, this->read);
                        // reset the first place of vector list
                        // Removing the first element
                        Spectatorlist.erase(Spectatorlist.begin());
                    }

                    if (gamePlayer == nullptr && Spectatorlist.size() == 0)
                    {

                        // terminate the thread if there is no player and the list is empty
                        // std::cout << "Thread has finished execution.\n";
                        // return 0;  then terminat
                        continueFlag = false;
                    }
                    else
                    {
                        continueFlag == true;
                    }
                }

                std::this_thread::sleep_for(std::chrono::seconds(10));
                std::cout << "Game room ending...\n";
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

    Semaphore write = Semaphore("write" + count, 1, true);
    Semaphore read = Semaphore("read" + count, 0, true);

    Shared<MyShared> sharedMemory("sharedMemory", true);
    // Accept connections and create game rooms
    try
    {
        while (true)
        {
            Socket newConnection = server.Accept();

            std::cout << "Accepted new connection. Starting a game room...\n";

            if (count < 3)
            {
                // Start a new game room for each connection
                gameRooms.push_back(new GameRoom(count, new Player(newConnection, count, Semaphore("write"), Semaphore("read")), Semaphore("write"), Semaphore("read")));
                count++;
            }
            else
            {

                // send message to client tell him the room is room full
                // which room you want to join

                // wait response
                //  if (out of range ---- retry) inside the range put to the spectator list

                // send current situation of current rooom
                std::cout << "Spectator" << std::endl;
                Spectator *newSpec = new Spectator(newConnection, Semaphore("read"));
                newSpec->askRoom();
                ByteArray response;
                newConnection.Read(response);
                std::string receive(response.v.begin(), response.v.end());
                if (receive.length() == 0)
                {
                    break;
                }
                read = Semaphore("read" + receive);
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