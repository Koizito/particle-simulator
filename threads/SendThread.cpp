#include "SendThread.hpp"

SendThread::SendThread(AppContext &inputAppCtx)
    : appCtx(inputAppCtx) {
}

void SendThread::startThread() {
    try {
        workerThread = std::thread([this]() {
            while (true) {
                std::unique_lock<std::mutex> sendLock(this->appCtx.sendThreadMutex);

                this->appCtx.checkIfSendThreadShouldRun.wait(sendLock, [this] {
                    return (this->appCtx.shouldSendThreadRun && !this->appCtx.normalSendQueue.empty()) || !this->appCtx.
                           highPrioritySendQueue.empty() || this->appCtx.shouldExit;
                });

                if (this->appCtx.shouldExit) break;

                OutgoingMessage message;

                if (!this->appCtx.highPrioritySendQueue.empty()) {
                    message = std::move(this->appCtx.highPrioritySendQueue.front());
                    this->appCtx.highPrioritySendQueue.pop();
                } else {
                    message = std::move(this->appCtx.normalSendQueue.front());
                    this->appCtx.normalSendQueue.pop();
                }

                this->appCtx.checkIfSimulationThreadShouldRun.notify_one();

                sendLock.unlock();

                if (auto client = this->appCtx.currentClient.load(); client) {
                    if (message.binary) {
                        ix::IXWebSocketSendData data(message.binaryData);
                        client->sendBinary(data);
                    } else {
                        client->send(message.textData);
                    }
                }
            }
        });
    } catch (const std::exception& e) {
        std::cerr << "Send thread error: " << e.what() << "\n";
        this->appCtx.signalExit();
    } catch (...) {
        std::cerr << "Send thread unknown error\n";
        this->appCtx.signalExit();
    }
}

void SendThread::stopThread() {
    if (workerThread.joinable()) {
        workerThread.join();
    }
}
