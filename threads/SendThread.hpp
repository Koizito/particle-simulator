#pragma once
#include "BaseThread.hpp"

class Sendthread : public BaseThread {

public:
    void runThread() override;
    void waitForStartSignal(std::unique_lock<std::mutex> sendLock);
    OutgoingMessage getNextMessage();
    void sendMessage(const OutgoingMessage& message);
};