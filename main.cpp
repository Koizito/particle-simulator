#include <atomic>
#include <IXWebSocketServer.h>
#include <mutex>

#include "app/AppContext.hpp"
#include "networking/NetworkingHandler.hpp"
#include "networking/MessageHandler.hpp"
#include "threads/SendThread.hpp"
#include "threads/SimulationThread.hpp"

int main() {
    AppContext appCtx;
    MessageHandler messageHandler(appCtx);
    NetworkingHandler networkingHandler(appCtx, messageHandler);

    if (!networkingHandler.startServer()) {
        return 1;
    }

    SendThread sendThread(appCtx);
    sendThread.startThread();

    SimulationThread simulationThread(appCtx);
    simulationThread.startThread();

    {
        std::unique_lock<std::mutex> lock(appCtx.shouldExitMutex);
        appCtx.checkIfShouldExit.wait(lock, [&appCtx] { return appCtx.shouldExit.load(); });
    }

    simulationThread.stopThread();
    sendThread.stopThread();

    networkingHandler.stopServer();

    return 0;
}
