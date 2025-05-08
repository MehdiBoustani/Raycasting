#ifndef A_H
#define A_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <UDPSender.h>
#include <Player.h>

class PositionSender {
    public:
        PositionSender(UDPSender &sender, Player &player);
        ~PositionSender();

        void notifyMovement();

    private:
        std::thread senderThread;
        std::mutex myMutex;
        std::condition_variable myConditionVariable;
        bool stop;
        bool moved;
        UDPSender& sender;
        Player& player;
};

#endif