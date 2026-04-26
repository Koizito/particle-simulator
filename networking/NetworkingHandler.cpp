#include <spdlog/spdlog.h>

#include "NetworkingHandler.hpp"
#include "MessageHandler.hpp"
#include "app/AppContext.hpp"

NetworkingHandler::NetworkingHandler(std::shared_ptr<spdlog::logger> inputLogger, AppContext& inputAppCtx,
                                     MessageHandler& inputMessageHandler)
    : logger(std::move(inputLogger)), appCtx(inputAppCtx), messageHandler(inputMessageHandler),
      server(8080, "0.0.0.0") {
}

bool NetworkingHandler::startServer() {
    ix::initNetSystem();
    server.setOnClientMessageCallback(
        [this](const std::shared_ptr<ix::ConnectionState>&,
               ix::WebSocket& webSocket,
               const std::unique_ptr<ix::WebSocketMessage>& msg) {
            if (msg->type == ix::WebSocketMessageType::Open) {
                ix::WebSocket* expected = nullptr;
                if (!this->appCtx.currentClient.compare_exchange_strong(expected, &webSocket)) {
                    this->logger->warn("A client is already connected to the server.");
                    webSocket.send("Server busy");
                    webSocket.close();
                    return;
                }
                this->logger->info("Client connected.");
            } else if (msg->type == ix::WebSocketMessageType::Close) {
                ix::WebSocket* expected = &webSocket;
                this->appCtx.currentClient.compare_exchange_strong(expected, nullptr);

                this->logger->info("Client disconnected.");
            } else if (msg->type == ix::WebSocketMessageType::Message) {
                messageHandler.handleMessage(msg->str);
            }
        }
    );

    auto result = server.listen();
    if (!result.first) {
        logger->error("Failed to listen on the server. Listen error: {}", result.second);
        return false;
    }

    server.start();

    logger->info("Server listening on port {}.", server.getPort());
    return true;
}

void NetworkingHandler::stopServer() {
    logger->info("Stopping server.");
    server.stop();
    ix::uninitNetSystem();
}
