#pragma once
#include <IXWebSocketServer.h>
#include <spdlog/spdlog.h>

class MessageHandler;
struct AppContext;

class NetworkingHandler {
    std::shared_ptr<spdlog::logger> logger;
    AppContext& appCtx;
    MessageHandler& messageHandler;
    ix::WebSocketServer server;

public:
    NetworkingHandler(std::shared_ptr<spdlog::logger> inputLogger, AppContext& inputAppCtx,
                      MessageHandler& inputMessageHandler);

    [[nodiscard]] bool startServer();

    void stopServer();
};
