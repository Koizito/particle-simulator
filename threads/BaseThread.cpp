#include "threads/BaseThread.hpp"

#include <utility>

BaseThread::BaseThread(std::shared_ptr<spdlog::logger> inputLogger, std::string inputThreadType,
                       AppContext& inputAppCtx)
    : logger(std::move(inputLogger)), threadType(std::move(inputThreadType)), appCtx(inputAppCtx) {
}

BaseThread::~BaseThread() {
    stopThread();
}

void BaseThread::startThread() {
    logger->info("[{}] Starting thread", threadType);
    workerThread = std::thread([this] {
        logger->info("[{}] Started thread", threadType);
        runThread();
        logger->info("[{}] Exiting thread", threadType);
    });
}

void BaseThread::stopThread() {
    logger->info("[{}] Stopping thread", threadType);
    if (workerThread.joinable()) {
        workerThread.join();
    }
    logger->info("[{}] Stopped thread", threadType);
}
