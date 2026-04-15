#pragma once
#include <IXWebSocketServer.h>
#include <spdlog/logger.h>

class MessageHandler;
struct AppContext;

class NetworkingHandler {
    std::shared_ptr<spdlog::logger> logger;
    AppContext& appCtx;
    MessageHandler& messageHandler;
    ix::WebSocketServer server;

public:
    explicit NetworkingHandler(AppContext& inputAppCtx, MessageHandler& inputMessageHandler);

    [[nodiscard]] bool startServer();

    void stopServer();
};
