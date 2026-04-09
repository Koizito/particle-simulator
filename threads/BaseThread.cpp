#include "threads/BaseThread.hpp"

BaseThread::BaseThread(AppContext& inputAppCtx)
    : appCtx(inputAppCtx) {
}

BaseThread::~BaseThread() {
    stopThread();
}

void BaseThread::startThread() {
    workerThread = std::thread([this] {
        runThread();
    });
}

void BaseThread::stopThread() {
    if (workerThread.joinable()) {
        workerThread.join();
    }
}
