#include "MessagingQueue.hpp"

#include <spdlog/spdlog.h>

MessagingQueue::MessagingQueue(std::shared_ptr<spdlog::logger> inputLogger, size_t inputMaxQueueSize)
    : logger(std::move(inputLogger)), MAX_QUEUE_SIZE(inputMaxQueueSize) {
}

void MessagingQueue::waitForSpaceInNormalQueue(
    const std::atomic<bool>& shouldRun,
    const std::atomic<bool>& shouldExit) {
    std::unique_lock<std::mutex> queueLock(queuesMutex);
    spaceAvailableCV.wait(queueLock, [&] {
        return normalPriorityQueue.size() < MAX_QUEUE_SIZE && shouldRun.load() || shouldExit.load();
    });
}

void MessagingQueue::waitForDataInQueues(
    const std::atomic<bool>& shouldRun,
    const std::atomic<bool>& shouldExit) {
    std::unique_lock<std::mutex> queueLock(queuesMutex);
    dataAvailableCV.wait(queueLock, [&] {
        return !highPriorityQueue.empty() ||
               (!normalPriorityQueue.empty() && shouldRun.load()) ||
               shouldExit.load();
    });
}

void MessagingQueue::pushMessageToNormalQueue(OutgoingMessage message) {
    logger->debug("Pushing message to normal priority queue");
    {
        std::lock_guard<std::mutex> queueLock(queuesMutex);
        normalPriorityQueue.push(std::move(message));
    }
    dataAvailableCV.notify_all();
}

void MessagingQueue::pushMessageToHighPriorityQueue(OutgoingMessage message) {
    logger->debug("Pushing message to high priority queue");
    {
        std::lock_guard<std::mutex> queueLock(queuesMutex);
        highPriorityQueue.push(std::move(message));
    }
    dataAvailableCV.notify_all();
}

OutgoingMessage MessagingQueue::getNextMessage() {
    logger->debug("Getting next message");
    OutgoingMessage message;
    {
        std::lock_guard<std::mutex> queueLock(queuesMutex);
        if (!highPriorityQueue.empty()) {
            message = std::move(highPriorityQueue.front());
            highPriorityQueue.pop();
        } else if (!normalPriorityQueue.empty()) {
            message = std::move(normalPriorityQueue.front());
            normalPriorityQueue.pop();
        }
    }
    spaceAvailableCV.notify_all();
    return message;
}

void MessagingQueue::notifyAll() {
    std::lock_guard<std::mutex> queueLock(queuesMutex);
    spaceAvailableCV.notify_all();
    dataAvailableCV.notify_all();
}