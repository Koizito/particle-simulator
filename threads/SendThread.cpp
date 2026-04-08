#include "SendThread.hpp"

SendThread::SendThread(AppContext& inputAppCtx)
    : BaseThread(inputAppCtx) {
}

void SendThread::runThread() {
    try {
        while (true) {
            std::unique_lock<std::mutex> sendLock(this->appCtx.sendThreadMutex);
            waitForStartSignal(sendLock);

            if (this->appCtx.shouldExit) break;

            OutgoingMessage message = getNextMessage();

            sendLock.unlock();

            if (message.textData.empty() && message.binaryData.empty()) {
                continue;
            }

            sendMessage(message);

            this->appCtx.checkIfSimulationThreadShouldRun.notify_one();
        }
    } catch (const std::exception& e) {
        std::cerr << "Send thread error: " << e.what() << "\n";
        this->appCtx.signalExit();
    } catch (...) {
        std::cerr << "Send thread unknown error\n";
        this->appCtx.signalExit();
    }
}

void SendThread::waitForStartSignal(std::unique_lock<std::mutex>& sendLock) const {
    this->appCtx.checkIfSendThreadShouldRun.wait(sendLock, [this] {
        return (this->appCtx.shouldSendThreadRun && !this->appCtx.normalSendQueue.empty()) || !this->appCtx.
               highPrioritySendQueue.empty() || this->appCtx.shouldExit;
    });
}

OutgoingMessage SendThread::getNextMessage() const {
    OutgoingMessage message;
    if (!this->appCtx.highPrioritySendQueue.empty()) {
        message = std::move(this->appCtx.highPrioritySendQueue.front());
        this->appCtx.highPrioritySendQueue.pop();
    } else if (!this->appCtx.normalSendQueue.empty()) {
        message = std::move(this->appCtx.normalSendQueue.front());
        this->appCtx.normalSendQueue.pop();
    }
    return message;
}

void SendThread::sendMessage(const OutgoingMessage& message) const {
    if (const auto client = this->appCtx.currentClient.load(); client) {
        if (message.binary) {
            const ix::IXWebSocketSendData data(message.binaryData);
            client->sendBinary(data);
        } else {
            client->send(message.textData);
        }
    }
}
