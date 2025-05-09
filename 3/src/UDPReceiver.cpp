#include <stdexcept>
#include <arpa/inet.h>
#include <unistd.h>

#include <UDPReceiver.h>

UDPReceiver::UDPReceiver(int port) : stillRunning(false), nextPlayerIdx(0), gameMap(nullptr), numPlayers(0)
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        throw std::runtime_error("Failed to create socket");

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (sockaddr *)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("Failed to bind socket");
}

UDPReceiver::~UDPReceiver()
{
    stopThread();
    close(sockfd);
}

void UDPReceiver::startThread(Map& map, int nbPlayers)
{
    // If the thread is already running, we don't start it again
    if (stillRunning)
        return;

    // We set the map and the number of players
    gameMap = &map;

    // We set the number of players
    numPlayers = nbPlayers;

    // Flag to control thread execution
    stillRunning = true;

    // We initialize the thread with the receiverThread function and the current object
    thread = std::thread(&UDPReceiver::receiverThread, this);
}

void UDPReceiver::stopThread()
{
    if (!stillRunning)
        return;

    stillRunning = false;
    if (thread.joinable())
        thread.join();
}

void UDPReceiver::receiverThread()
{
    while(stillRunning)
    {
        UDPData data = receive();
        if (!data.valid)
            continue;

        std::lock_guard<std::mutex> lock(playerMutex);

        // If the player is not in the map, we add it
        if(playerIdx.find(data.sender) == playerIdx.end())
        {
            // We add the player to the map
            playerIdx[data.sender] = nextPlayerIdx++;

            // We make sure the index is not greater than the number of players
            nextPlayerIdx %= numPlayers;
        }

        // We get the index of the player
        int index = playerIdx[data.sender];

        // We move the player to the new position
        gameMap->movePlayer(index, data.position.x(), data.position.y());
    }
}

UDPData UDPReceiver::receive()
{
    socklen_t len = sizeof(addr);
    int read = recvfrom(sockfd, buffer, bufferSize, MSG_WAITALL, (sockaddr *)&addr, &len);

    if (read != bufferSize)
        return {false, "", {0, 0}};

    return {
        true,
        std::string(inet_ntoa(addr.sin_addr)) + std::to_string(addr.sin_port),
        {buffer[0], buffer[1]}};
}
