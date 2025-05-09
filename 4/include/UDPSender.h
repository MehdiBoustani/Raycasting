#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <netinet/in.h>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

/**
 * @brief The UDPSender class is responsible for sending position data using the UDP protocol.
 */
class UDPSender
{
public:
    /**
     * @brief Constructs a UDPSender object with the specified destination IP address and port.
     *
     * @param ip The IP address to send the packets to.
     * @param port The port number to send the packets to.
     */
    UDPSender(std::string ip, int port);

    /**
     * @brief Destroys the UDPSender object and closes the socket.
     */
    ~UDPSender();

    /**
     * @brief Starts the sender thread.
     */
    void startThread();

    /**
     * @brief Stops the sender thread.
     */
    void stopThread();

    /**
     * @brief Notifies the sender thread that the position has changed.
     *
     * @param x The x coordinate to send.
     * @param y The y coordinate to send.
     */
    void notifyPositionChanged(double x, double y);

private:
    void senderThread();

    int sockfd;                          // The socket file descriptor.
    double buffer[2];                    // The buffer to store the coordinates.
    sockaddr_in addr;                    // The address structure for the destination IP and port.
    int bufferSize = 2 * sizeof(double); // The size of the buffer.

    std::thread thread;                  // The sender thread
    std::mutex mutex;                    // Mutex for protecting shared data
    std::condition_variable condition;   // Condition variable for thread synchronization
    std::atomic<bool> stillRunning;      // Flag to control thread execution (atomic to avoid race conditions)
    
    // Current position to send
    double currentX;
    double currentY;
    bool positionChanged;                // Flag indicating if position has changed
};

#endif