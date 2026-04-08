#pragma once
#include <IXWebSocketServer.h>
#include <iostream>

#include "app/AppContext.hpp"

class NetworkingHandler {
    AppContext& appCtx;
    ix::WebSocketServer server;

public:
    explicit NetworkingHandler(AppContext& inputAppCtx);

    [[nodiscard]] bool startServer();

    void stopServer();
};
