#include "AppContext.hpp"

AppContext::AppContext(int input_maxStepsPerFrame, size_t input_maxQueueSize)
    : MAX_STEPS_PER_FRAME(input_maxStepsPerFrame), MAX_QUEUE_SIZE(input_maxQueueSize) {
}

void AppContext::signalExit() {
    shouldExit.store(true);
    shouldSimulationThreadRun.store(true);
    shouldSendThreadRun.store(true);

    checkIfSimulationThreadShouldRun.notify_one();
    checkIfSendThreadShouldRun.notify_one();

    checkIfShouldExit.notify_one();
}