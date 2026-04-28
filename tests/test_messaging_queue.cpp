#include <catch2/catch_test_macros.hpp>

#include <spdlog/spdlog.h>
#include "spdlog/sinks/stdout_color_sinks.h"
#include "app/MessagingQueue.hpp"

static auto testLogger = spdlog::stdout_color_mt("test");

TEST_CASE("waitForSpaceInNormalQueue returns immediately when space available and shouldRun=true", "[queue]") {
    MessagingQueue queue(testLogger, 10);
    const std::atomic<bool> shouldRun{true};
    const std::atomic<bool> shouldExit{false};

    const auto start = std::chrono::steady_clock::now();
    queue.waitForSpaceInNormalQueue(shouldRun, shouldExit);
    const auto end = std::chrono::steady_clock::now();
    REQUIRE((end - start) < std::chrono::milliseconds(10));
}

TEST_CASE("waitForSpaceInNormalQueue blocks when queue full and shouldExit false", "[queue][blocking]") {
    const size_t MAX = 3;
    MessagingQueue queue(testLogger, MAX);
    const std::atomic<bool> shouldRun{true};
    const std::atomic<bool> shouldExit{false};

    for (size_t i = 0; i < MAX; ++i) {
        queue.pushMessageToNormalQueue(OutgoingMessage{"msg"});
    }

    std::atomic<bool> waken{false};
    std::thread waiter([&]{
        queue.waitForSpaceInNormalQueue(shouldRun, shouldExit);
        waken = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(waken);

    const auto msg = queue.getNextMessage();
    REQUIRE_FALSE(msg.textData.empty());

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(waken);

    waiter.join();
}

TEST_CASE("waitForSpaceInNormalQueue unblocks on shouldExit", "[queue][exit]") {
    const size_t MAX = 3;
    MessagingQueue queue(testLogger, MAX);
    const std::atomic<bool> shouldRun{false};
    std::atomic<bool> shouldExit{false};

    for (size_t i = 0; i < MAX; ++i) {
        queue.pushMessageToNormalQueue(OutgoingMessage{"msg"});
    }

    std::atomic<bool> waken{false};
    std::thread waiter([&]{
        queue.waitForSpaceInNormalQueue(shouldRun, shouldExit);
        waken = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(waken);

    shouldExit = true;
    queue.notifyAll();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(waken);
    waiter.join();
}

TEST_CASE("waitForSpaceInNormalQueue blocks when shouldRun false and shouldExit false", "[queue]") {
    MessagingQueue queue(testLogger, 10);
    const std::atomic<bool> shouldRun{false};
    std::atomic<bool> shouldExit{false};

    std::atomic<bool> waken{false};
    std::thread waiter([&]{
        queue.waitForSpaceInNormalQueue(shouldRun, shouldExit);
        waken = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(waken);

    shouldExit = true;
    queue.notifyAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(waken);
    waiter.join();
}