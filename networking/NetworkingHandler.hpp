#pragma once
#include <IXWebSocketServer.h>

#include "app/AppContext.hpp"
#include "networking/MessageHandler.hpp"

class NetworkingHandler {
    AppContext& appCtx;
    MessageHandler& messageHandler;
    ix::WebSocketServer server;

public:
    explicit NetworkingHandler(AppContext& inputAppCtx, MessageHandler& inputMessageHandler);

    [[nodiscard]] bool startServer();

    void stopServer();
};
