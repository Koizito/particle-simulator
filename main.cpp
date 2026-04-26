#include <atomic>
#include <IXWebSocketServer.h>
#include <mutex>

#include "app/AppContext.hpp"
#include "networking/NetworkingHandler.hpp"
#include "networking/MessageHandler.hpp"
#include "threads/SendThread.hpp"
#include "threads/SimulationThread.hpp"
#include "spdlog/spdlog.h"
#include <spdlog/cfg/env.h>
#include "spdlog/sinks/stdout_color_sinks.h"

int main() {
    spdlog::cfg::load_env_levels();
    spdlog::set_pattern("[%H:%M:%S] [%^%l%$] [tid %t] %v");
    const auto mainLogger = spdlog::stdout_color_mt("main");
    const auto networkingLogger = spdlog::stdout_color_mt("networkingHandler");
    const auto messageLogger = spdlog::stdout_color_mt("messageHandler");
    const auto appCtxLogger = spdlog::stdout_color_mt("appContext");
    const auto messageQueueLogger = spdlog::stdout_color_mt("messageQueue");
    const auto threadsLogger = spdlog::stdout_color_mt("threads");

    AppContext appCtx(appCtxLogger, messageQueueLogger);
    MessageHandler messageHandler(messageLogger, appCtx);
    NetworkingHandler networkingHandler(networkingLogger, appCtx, messageHandler);
    mainLogger->info("Starting server...");
    if (!networkingHandler.startServer()) {
        mainLogger->error("Failed to start server");
        return 1;
    }
    mainLogger->info("Starting Threads...");
    SendThread sendThread(threadsLogger, appCtx);
    sendThread.startThread();

    SimulationThread simulationThread(threadsLogger, appCtx);
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
