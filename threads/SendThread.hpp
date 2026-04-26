#pragma once
#include <mutex>
#include "threads/BaseThread.hpp"

struct AppContext;
struct OutgoingMessage;

class SendThread : public BaseThread {
public:
    SendThread(std::shared_ptr<spdlog::logger> inputLogger, AppContext& inputAppCtx);

    void runThread() override;

    void waitForRunSignal() const;

    [[nodiscard]] OutgoingMessage getNextMessage() const;

    void sendMessage(const OutgoingMessage& message) const;
};
