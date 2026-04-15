#include <atomic>
#include <IXWebSocketServer.h>
#include <mutex>

#include "app/AppContext.hpp"
#include "networking/NetworkingHandler.hpp"
#include "networking/MessageHandler.hpp"
#include "threads/SendThread.hpp"
#include "threads/SimulationThread.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

int main() {
    const auto mainLogger = spdlog::stdout_color_mt("main");
    spdlog::stdout_color_mt("networkingHandler");
    AppContext appCtx;
    MessageHandler messageHandler(appCtx);
    NetworkingHandler networkingHandler(appCtx, messageHandler);
    mainLogger->info("Starting server...");
    if (!networkingHandler.startServer()) {
        mainLogger->error("Failed to start server");
        return 1;
    }
    mainLogger->info("Starting Threads...");
    SendThread sendThread(appCtx);
    sendThread.startThread();

    SimulationThread simulationThread(appCtx);
    simulationThread.startThread();

    {
        std::unique_lock<std::mutex> lock(appCtx.shouldExitMutex);
        mainLogger->info("Application running. Waiting for shutdown signal.");
        appCtx.checkIfShouldExit.wait(lock, [&appCtx] { return appCtx.shouldExit.load(); });
    }

    mainLogger->info("Shutdown signal received. Shutting down application...");
    simulationThread.stopThread();
    sendThread.stopThread();
    mainLogger->info("Threads Stopped.");
    networkingHandler.stopServer();
    mainLogger->info("Server Stopped.");
    return 0;
}
