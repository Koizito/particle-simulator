#pragma once
#include <mutex>
#include <queue>
#include <spdlog/spdlog.h>
#include "core/OutgoingMessage.hpp"

class MessagingQueue {
    std::shared_ptr<spdlog::logger> logger;

    std::mutex queuesMutex;

    std::condition_variable spaceAvailableCV;
    std::condition_variable dataAvailableCV;

    std::queue<OutgoingMessage> highPriorityQueue;
    std::queue<OutgoingMessage> normalPriorityQueue;

    size_t MAX_QUEUE_SIZE;

public:
    explicit MessagingQueue(std::shared_ptr<spdlog::logger> inputLogger, size_t inputMaxQueueSize = 100);

    void waitForSpaceInNormalQueue(
    const std::atomic<bool>& shouldRun,
    const std::atomic<bool>& shouldExit);

    void waitForDataInQueues(
    const std::atomic<bool>& shouldRun,
    const std::atomic<bool>& shouldExit);

    void pushMessageToNormalQueue(OutgoingMessage message);

    void pushMessageToHighPriorityQueue(OutgoingMessage message);

    OutgoingMessage getNextMessage();

    void notifyAll();
};
