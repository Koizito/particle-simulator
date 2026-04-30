#include <catch2/catch_test_macros.hpp>

#include "spdlog/sinks/stdout_color_sinks.h"
#include "app/MessagingQueue.hpp"
#include "ThreadJoiner.hpp"

static auto testLogger = spdlog::stdout_color_mt("test");

TEST_CASE("getNextMessage returns empty message when queues are empty", "[queue]") {
    MessagingQueue queue(testLogger);
    OutgoingMessage emptyMessage = queue.getNextMessage();
    REQUIRE(emptyMessage.textData.empty());
    REQUIRE(emptyMessage.binaryData.empty());
}

TEST_CASE("Using pushMessageToNormalQueue pushes an OutgoingMessage into the normal priority queue", "[queue][pushtonormalqueue]") {
    MessagingQueue queue(testLogger);

    auto emptyMessage = queue.getNextMessage();

    REQUIRE(emptyMessage.textData.empty());
    REQUIRE(emptyMessage.binaryData.empty());

    std::string inputTextData = "testNormalMessage";
    queue.pushMessageToNormalQueue(OutgoingMessage(inputTextData));

    auto filledMessage = queue.getNextMessage();

    REQUIRE(filledMessage.textData == inputTextData);
    REQUIRE(filledMessage.binaryData.empty());
}

TEST_CASE("Using pushMessageToHighPriorityQueue pushes an OutgoingMessage into the high priority queue", "[queue][pushtohighpriorityqueue]") {
    MessagingQueue queue(testLogger);

    auto emptyMessage = queue.getNextMessage();

    REQUIRE(emptyMessage.textData.empty());
    REQUIRE(emptyMessage.binaryData.empty());

    std::string inputTextData = "testPriorityMessage";
    queue.pushMessageToHighPriorityQueue(OutgoingMessage(inputTextData));

    auto filledMessage = queue.getNextMessage();

    REQUIRE(filledMessage.textData == inputTextData);
    REQUIRE(filledMessage.binaryData.empty());
}

TEST_CASE("High priority messages are returned first when calling getNextMessage", "[queue][pushtonormalqueue][pushtohighpriorityqueue]") {
    MessagingQueue queue(testLogger);

    std::string inputTextDataNormal = "testNormalMessage";
    queue.pushMessageToNormalQueue(OutgoingMessage(inputTextDataNormal));

    std::string inputTextDataPriority = "testPriorityMessage";
    queue.pushMessageToHighPriorityQueue(OutgoingMessage(inputTextDataPriority));

    auto highPriorityMessage = queue.getNextMessage();

    REQUIRE(highPriorityMessage.textData == inputTextDataPriority);
    REQUIRE(highPriorityMessage.binaryData.empty());

    auto normalPriorityMessage = queue.getNextMessage();

    REQUIRE(normalPriorityMessage.textData == inputTextDataNormal);
    REQUIRE(normalPriorityMessage.binaryData.empty());
}

TEST_CASE("waitForSpaceInNormalQueue returns immediately when space available and shouldRun=true", "[queue][spaceavailablecv]") {
    MessagingQueue queue(testLogger);
    const std::atomic<bool> shouldRun{true};
    const std::atomic<bool> shouldExit{false};

    const auto start = std::chrono::steady_clock::now();
    queue.waitForSpaceInNormalQueue(shouldRun, shouldExit);
    const auto end = std::chrono::steady_clock::now();
    REQUIRE((end - start) < std::chrono::milliseconds(10));
}

TEST_CASE("waitForSpaceInNormalQueue blocks when queue full and shouldExit false", "[queue][spaceavailablecv]") {
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
        waken.store(true);
    });
    ThreadJoiner guard(waiter);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(waken.load());

    const auto msg = queue.getNextMessage();
    REQUIRE_FALSE(msg.textData.empty());

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(waken.load());
}

TEST_CASE("waitForSpaceInNormalQueue unblocks on shouldExit", "[queue][spaceavailablecv]") {
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
        waken.store(true);
    });
    ThreadJoiner guard(waiter);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(waken.load());

    shouldExit.store(true);
    queue.notifyAll();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(waken.load());
}

TEST_CASE("waitForSpaceInNormalQueue blocks when shouldRun false and shouldExit false", "[queue][spaceavailablecv]") {
    MessagingQueue queue(testLogger);
    const std::atomic<bool> shouldRun{false};
    std::atomic<bool> shouldExit{false};

    std::atomic<bool> waken{false};
    std::thread waiter([&]{
        queue.waitForSpaceInNormalQueue(shouldRun, shouldExit);
        waken.store(true);
    });
    ThreadJoiner guard(waiter);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(waken.load());

    shouldExit.store(true);
    queue.notifyAll();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(waken.load());
}

TEST_CASE("waitForDataInQueues returns when highPriorityQueue receives object", "[queue][dataavailablecv]") {
    MessagingQueue queue(testLogger);
    const std::atomic<bool> shouldRun{ false };
    const std::atomic<bool> shouldExit{ false };

    std::atomic<bool> waken{ false };
    std::thread waiter([&] {
        queue.waitForDataInQueues(shouldRun, shouldExit);
        waken.store(true);
        });
    ThreadJoiner guard(waiter);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(waken.load());

    queue.pushMessageToHighPriorityQueue(OutgoingMessage{ "msg" });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(waken.load());
}

TEST_CASE("waitForDataInQueues blocks when queues are empty and shouldExit false", "[queue][dataavailablecv]") {
    MessagingQueue queue(testLogger);
    const std::atomic<bool> shouldRun{ true };
    const std::atomic<bool> shouldExit{ false };

    std::atomic<bool> waken{ false };
    std::thread waiter([&] {
        queue.waitForDataInQueues(shouldRun, shouldExit);
        waken.store(true);
        });
    ThreadJoiner guard(waiter);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(waken.load());

    queue.pushMessageToNormalQueue(OutgoingMessage{ "msg" });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(waken.load());
}

TEST_CASE("waitForDataInQueues blocks when shouldRun and shouldExit false", "[queue][dataavailablecv]") {
    MessagingQueue queue(testLogger);
    std::atomic<bool> shouldRun{ false };
    const std::atomic<bool> shouldExit{ false };

    queue.pushMessageToNormalQueue(OutgoingMessage{ "msg" });

    std::atomic<bool> waken{ false };
    std::thread waiter([&] {
        queue.waitForDataInQueues(shouldRun, shouldExit);
        waken.store(true);
        });
    ThreadJoiner guard(waiter);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(waken.load());

    shouldRun.store(true);
    queue.notifyAll();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(waken.load());
}

TEST_CASE("waitForDataInQueues blocks when normal queue not empty, shouldRun false and shouldExit false. Unblocks on shouldExit true", "[queue][dataavailablecv]") {
    MessagingQueue queue(testLogger);
    const std::atomic<bool> shouldRun{ false };
    std::atomic<bool> shouldExit{ false };

    queue.pushMessageToNormalQueue(OutgoingMessage{ "msg" });

    std::atomic<bool> waken{ false };
    std::thread waiter([&] {
        queue.waitForDataInQueues(shouldRun, shouldExit);
        waken.store(true);
        });
    ThreadJoiner guard(waiter);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(waken.load());

    shouldExit.store(true);
    queue.notifyAll();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(waken.load());
}

TEST_CASE("waitForDataInQueues blocks when normal queue empty, shouldRun true and shouldExit false. Unblocks on shouldExit true", "[queue][dataavailablecv]") {
    MessagingQueue queue(testLogger);
    const std::atomic<bool> shouldRun{ true };
    std::atomic<bool> shouldExit{ false };

    std::atomic<bool> waken{ false };
    std::thread waiter([&] {
        queue.waitForDataInQueues(shouldRun, shouldExit);
        waken.store(true);
        });
    ThreadJoiner guard(waiter);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(waken.load());

    shouldExit.store(true);
    queue.notifyAll();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(waken.load());
}