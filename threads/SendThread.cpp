#include "threads/SendThread.hpp"
#include <mutex>
#include <utility>
#include "app/AppContext.hpp"
#include "IXWebSocket.h"
#include "core/OutgoingMessage.hpp"

SendThread::SendThread(std::shared_ptr<spdlog::logger> inputLogger, AppContext& inputAppCtx)
    : BaseThread(std::move(inputLogger), "SendThread", inputAppCtx) {
}

void SendThread::runThread() {
    try {
        logger->debug("[{}] Send loop started", threadType);
        while (true) {
            this->appCtx.messagingQueue.waitForDataInQueues(appCtx.shouldSendThreadRun, appCtx.shouldExit);

            if (this->appCtx.shouldExit) break;

            OutgoingMessage message = this->appCtx.messagingQueue.getNextMessage();

            if (message.textData.empty() && message.binaryData.empty()) {
                logger->debug("[{}] No message found to send", threadType);
                continue;
            }

            sendMessage(message);
        }
        logger->debug("[{}] Send loop ended", threadType);
    } catch (const std::exception& e) {
        logger->error("[{}] Error: {}", threadType, e.what());
        this->appCtx.signalExit();
    } catch (...) {
        logger->error("[{}] Unknown error", threadType);
        this->appCtx.signalExit();
    }
}

void SendThread::sendMessage(const OutgoingMessage& message) const {
    logger->debug("[{}] Sending message", threadType);
    const auto client = this->appCtx.currentClient.load();
    if (!client) return;

    if (!message.textData.empty()) {
        client->send(message.textData);
    }
    if (!message.binaryData.empty()) {
        client->sendBinary(ix::IXWebSocketSendData(message.binaryData));
    }
}
