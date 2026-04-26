#pragma once
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "core/World.hpp"
#include <spdlog/spdlog.h>

#include "app/MessagingQueue.hpp"

namespace ix {
    class WebSocket;
}

struct AppContext {
    std::shared_ptr<spdlog::logger> logger;
    // Single client pointer
    std::atomic<ix::WebSocket*> currentClient{nullptr};
    // The single World object
    World mainWorld;
    // Messaging Queue object
    MessagingQueue messagingQueue;
    // Atomic int counter for the Particles ID assignment
    std::atomic<int> particleIdCounter = 1;
    // Should the continuous simulation be running
    std::atomic<bool> shouldSimulationThreadRun = false;
    // Should the send thread be running
    std::atomic<bool> shouldSendThreadRun = false;
    // Should the continuous simulation be exited
    std::atomic<bool> shouldExit = false;
    // Mutex to control the mainWorld access
    std::mutex worldMutex;
    // Condition variable to control the simulation thread
    std::mutex simulationThreadMutex;
    std::condition_variable checkIfSimulationThreadShouldRun;
    // Mutex and condition variable to stop everything
    std::mutex shouldExitMutex;
    std::condition_variable checkIfShouldExit;

    explicit AppContext(std::shared_ptr<spdlog::logger> inputAppCtxLogger,
                        std::shared_ptr<spdlog::logger> inputMessageQueueLogger);

    void notifyThreads();

    void setSimulationState(bool runSimulationThread, bool runSendThread);

    void signalExit();
};
