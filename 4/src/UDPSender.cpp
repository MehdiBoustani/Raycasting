#include <sys/socket.h>
#include <stdexcept>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <UDPSender.h>

UDPSender::UDPSender(std::string ip, int port)
    : stillRunning(false), currentX(0), currentY(0), positionChanged(false)
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        throw std::runtime_error("Failed to create socket");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
}

UDPSender::~UDPSender()
{
    stopThread();
    close(sockfd);
}

void UDPSender::startThread()
{
    // If the thread is already running, we don't start it again
    if (stillRunning)
        return;

    stillRunning = true;

    // We initialize the thread with the senderThread function and the current object
    thread = std::thread(&UDPSender::senderThread, this);
}

void UDPSender::stopThread()
{
    if (!stillRunning)
        return;

    {
        // Mutex to protect the stillRunning flag
        std::lock_guard<std::mutex> lock(mutex);
        stillRunning = false;
    }
    condition.notify_one();
    
    if (thread.joinable())
        thread.join();
}

void UDPSender::notifyPositionChanged(double x, double y)
{
    std::lock_guard<std::mutex> lock(mutex);
    currentX = x;
    currentY = y;
    positionChanged = true;
    condition.notify_one();
}

void UDPSender::senderThread()
{
    while (true)
    {
        // We used a unique_lock to avoid deadlocks when waiting for the condition variable.
        // The simple lock guard would have been enough, but we wanted to be sure that the lock is released when the thread is waiting.
        std::unique_lock<std::mutex> lock(mutex);
        
        // Wait until position changes or thread should stop
        condition.wait(lock, [this] { return positionChanged || !stillRunning; });
        
        // If the thread should stop, we break the loop
        if (!stillRunning)
            break;

        // Send the current position
        buffer[0] = currentX;
        buffer[1] = currentY;
        sendto(sockfd, buffer, bufferSize, 0, (sockaddr*)&addr, sizeof(addr));

        positionChanged = false;
    }
}