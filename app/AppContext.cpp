#include "AppContext.hpp"

#include <spdlog/spdlog.h>

#include <utility>

AppContext::AppContext(std::shared_ptr<spdlog::logger> inputAppCtxLogger,
                       std::shared_ptr<spdlog::logger> inputMessageQueueLogger)
    : logger(std::move(inputAppCtxLogger)), messagingQueue(std::move(inputMessageQueueLogger)) {
}

void AppContext::setSimulationState(const bool runSimulationThread, const bool runSendThread) {
    logger->debug("Should Simulation Thread run: {}", runSimulationThread);
    logger->debug("Should Send Thread run: {}", runSendThread);

    shouldSimulationThreadRun.store(runSimulationThread);
    shouldSendThreadRun.store(runSendThread);

    messagingQueue.notifyAll();
}

void AppContext::signalExit() {
    logger->info("Signaling Exit");
    shouldExit.store(true);
    messagingQueue.notifyAll();
    checkIfShouldExit.notify_all();
}
