#include <iostream>

#include "NetworkingHandler.hpp"

NetworkingHandler::NetworkingHandler(AppContext& inputAppCtx, MessageHandler& inputMessageHandler)
    : appCtx(inputAppCtx), messageHandler(inputMessageHandler), server(8080, "0.0.0.0") {
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
                    webSocket.send("Server busy");
                    webSocket.close();
                    return;
                }
                std::cout << "Client connected\n";
            } else if (msg->type == ix::WebSocketMessageType::Close) {
                ix::WebSocket* expected = &webSocket;
                this->appCtx.currentClient.compare_exchange_strong(expected, nullptr);

                std::cout << "Client disconnected\n";
            } else if (msg->type == ix::WebSocketMessageType::Message) {
                messageHandler.handleMessage(msg->str);
            }
        }
    );

    auto result = server.listen();
    if (!result.first) {
        std::cerr << "Listen error: " << result.second << "\n";
        return false;
    }

    server.start();
    std::cout << "Continuous simulation server running on port 8080\n";
    return true;
}

void NetworkingHandler::stopServer() {
    server.stop();
    ix::uninitNetSystem();
}
