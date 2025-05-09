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
    if (stillRunning)
        return;

    gameMap = &map;
    numPlayers = nbPlayers;
    stillRunning = true;
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
        if(playerIdx.find(data.sender) == playerIdx.end())
        {
            playerIdx[data.sender] = nextPlayerIdx++;
            nextPlayerIdx %= numPlayers;
        }

        int index = playerIdx[data.sender];
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
