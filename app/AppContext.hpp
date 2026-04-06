#pragma once
#include <atomic>
#include <IXWebSocket.h>
#include <queue>
#include "core/OutgoingMessage.hpp"
#include "core/World.hpp"

struct AppContext {
    // Single client pointer
    std::atomic<ix::WebSocket*> currentClient{nullptr};
    // The single world object
    World mainWorld;
    // Should the continuous simulation be running
    std::atomic<bool> shouldSimulationThreadRun = false;
    // Should the send thread be running
    std::atomic<bool> shouldSendThreadRun = false;
    // Should the continuous simulation be exited
    std::atomic<bool> shouldExit = false;
    // Mutex to control the mainWorld access
    std::mutex worldMutex;
    // Mutex and condition variable to control the simulation thread
    std::mutex simulationThreadMutex;
    std::condition_variable checkIfSimulationThreadShouldRun;
    // Mutex and condition variable to control the send thread
    std::mutex sendThreadMutex;
    std::condition_variable checkIfSendThreadShouldRun;
    // Condition variable to stop everything
    std::mutex shouldExitMutex;
    std::condition_variable checkIfShouldExit;
    // Queues where to keep the messages to send
    std::queue<OutgoingMessage> highPrioritySendQueue;
    std::queue<OutgoingMessage> normalSendQueue;

    // Max steps each frame of the simulation can attempt to make before synchronization is forced
    int MAX_STEPS_PER_FRAME = 10;
    // Max number of simulation entries in the queue before the simulation is paused
    size_t MAX_QUEUE_SIZE = 100;

    AppContext() = default;
    AppContext(int input_maxStepsPerFrame, size_t input_maxQueueSize);

    void signalExit();
};
