#include "AppContext.hpp"

AppContext::AppContext(const int inputMaxStepsPerFrame, const size_t inputMaxQueueSize)
    : MAX_STEPS_PER_FRAME(inputMaxStepsPerFrame), MAX_QUEUE_SIZE(inputMaxQueueSize) {
}

void AppContext::signalExit() {
    shouldExit.store(true);
    shouldSimulationThreadRun.store(true);
    shouldSendThreadRun.store(true);

    checkIfSimulationThreadShouldRun.notify_one();
    checkIfSendThreadShouldRun.notify_one();

    checkIfShouldExit.notify_one();
}
