#pragma once
#include "BaseThread.hpp"
#include <iostream>

class SendThread : public BaseThread {
public:
    explicit SendThread(AppContext& inputAppCtx);

    void runThread() override;

    void waitForStartSignal(std::unique_lock<std::mutex>& sendLock) const;

    OutgoingMessage getNextMessage() const;

    void sendMessage(const OutgoingMessage& message) const;
};
