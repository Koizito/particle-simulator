#include "AppContext.hpp"

AppContext::AppContext(int input_maxStepsPerFrame, size_t input_maxQueueSize)
    : MAX_STEPS_PER_FRAME(input_maxStepsPerFrame), MAX_QUEUE_SIZE(input_maxQueueSize) {
}

void AppContext::signalExit() {
    shouldExit.store(true);
    shouldSimulationThreadRun.store(true);
    shouldSendThreadRun.store(true);

    checkIfSimulationThreadShouldRun.notify_all();
    checkIfSendThreadShouldRun.notify_all();

    checkIfShouldExit.notify_one();
}