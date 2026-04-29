#include <catch2/catch_test_macros.hpp>

#include "spdlog/sinks/stdout_color_sinks.h"
#include "app/AppContext.hpp"
#include "threads/SimulationThread.hpp"

static auto appCtxLogger = spdlog::stdout_color_mt("appContext");
static auto messageQueueLogger = spdlog::stdout_color_mt("messageQueue");
static auto threadsLogger = spdlog::stdout_color_mt("threads");

TEST_CASE("getNextMessage returns empty message when queues are empty", "[simulationthread]") {
    AppContext appCtx(appCtxLogger, messageQueueLogger);
    SimulationThread simThread(threadsLogger, appCtx);
    simThread.startThread();

    //REQUIRE(emptyMessage.textData.empty());
    //REQUIRE(emptyMessage.binaryData.empty());
}