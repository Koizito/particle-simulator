#include "AppContext.hpp"

AppContext::AppContext(const int inputMaxStepsPerFrame, const size_t inputMaxQueueSize)
    : MAX_STEPS_PER_FRAME(inputMaxStepsPerFrame), MAX_QUEUE_SIZE(inputMaxQueueSize) {
}

void AppContext::notifyThreads() {
    checkIfSimulationThreadShouldRun.notify_one();
    checkIfSendThreadShouldRun.notify_one();
}

void AppContext::setSimulationState(bool runSimulationThread, bool runSendThread) {
    shouldSimulationThreadRun.store(runSimulationThread);
    shouldSendThreadRun.store(runSendThread);
    notifyThreads();
}

void AppContext::signalExit() {
    shouldExit.store(true);
    setSimulationState(true, true);
    notifyThreads();
    checkIfShouldExit.notify_one();
}
