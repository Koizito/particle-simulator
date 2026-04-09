#pragma once
#include <mutex>
#include "threads/BaseThread.hpp"

class AppContext;
class OutgoingMessage;

class SendThread : public BaseThread {
public:
    explicit SendThread(AppContext& inputAppCtx);

    void runThread() override;

    void waitForStartSignal(std::unique_lock<std::mutex>& sendLock) const;

    [[nodiscard]] OutgoingMessage getNextMessage() const;

    void sendMessage(const OutgoingMessage& message) const;
};
