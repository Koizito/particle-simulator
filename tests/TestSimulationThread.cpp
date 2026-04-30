#include <iostream>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fstream>

#include "spdlog/sinks/stdout_color_sinks.h"
#include <nlohmann/json.hpp>
#include "app/AppContext.hpp"
#include "threads/SimulationThread.hpp"
#include "DecodedParticle.hpp"

static auto appCtxLogger = spdlog::stdout_color_mt("appContext");
static auto messageQueueLogger = spdlog::stdout_color_mt("messageQueue");
static auto threadsLogger = spdlog::stdout_color_mt("threads");

TEST_CASE("Simulation snapshot after 5 steps is correct", "[simulationthread]") {
    AppContext appCtx(appCtxLogger, messageQueueLogger);
    SimulationThread simThread(threadsLogger, appCtx);

    {
        std::lock_guard lock(appCtx.worldMutex);
        appCtx.mainWorld.particles = {
            Particle{1, 1.0f, 10.0f, 20.0f, 30.0f, -1.0f, 10.0f, 2.0f},
            Particle{2, 2.0f, 15.0f, 5.0f, 25.0f, 5.0f, -1.0f, -2.0f}
        };
        appCtx.mainWorld.maxX = 50.0f;
        appCtx.mainWorld.maxY = 50.0f;
        appCtx.mainWorld.maxZ = 50.0f;
        appCtx.mainWorld.dt = 1.0f / 60.0f;
        appCtx.mainWorld.gravityAccel = -9.81f;
    }

    const int numberOfSteps = 5;
    for (int i = 0; i < numberOfSteps; ++i) {
        appCtx.mainWorld.step();
    }

    std::vector<Particle> particlesSnapshot = simThread.getParticlesSnapshot();

    nlohmann::json metadata = SimulationThread::getSnapshotMetadata(particlesSnapshot);
    std::vector<uint8_t> bufferBytes = SimulationThread::prepareSnapshotForSending(particlesSnapshot);

    simThread.queueForSending(metadata, bufferBytes);

    OutgoingMessage outSnapshot = appCtx.messagingQueue.getNextMessage();
    REQUIRE(!outSnapshot.textData.empty());
    REQUIRE(!outSnapshot.binaryData.empty());

    std::string outMetadata = outSnapshot.textData;
    nlohmann::json jsonMetadata = nlohmann::json::parse(outMetadata);

    REQUIRE(jsonMetadata["type"] == "particles");
    REQUIRE(jsonMetadata["count"] == 2);

    auto particles = decodeSnapshot(outSnapshot.binaryData);
    REQUIRE(particles.size() == 2);

    std::ifstream expectedFile("5_steps_expected_snapshot.json");
    REQUIRE(expectedFile.is_open());
    nlohmann::json expectedJson = nlohmann::json::parse(expectedFile);

    REQUIRE(particlesSnapshot.size() == expectedJson.size());

    using Catch::Matchers::WithinAbs;

    for (size_t i = 0; i < particlesSnapshot.size(); ++i) {
        const auto& expected = expectedJson[i];
        const auto& actual = particlesSnapshot[i];

        REQUIRE(actual.id == expected["id"]);
        REQUIRE_THAT(actual.mass, WithinAbs(expected["mass"], 1e-5));
        REQUIRE_THAT(actual.x,    WithinAbs(expected["x"],    1e-5));
        REQUIRE_THAT(actual.y,    WithinAbs(expected["y"],    1e-5));
        REQUIRE_THAT(actual.z,    WithinAbs(expected["z"],    1e-5));
        REQUIRE_THAT(actual.velX, WithinAbs(expected["velX"], 1e-5));
        REQUIRE_THAT(actual.velY, WithinAbs(expected["velY"], 1e-5));
        REQUIRE_THAT(actual.velZ, WithinAbs(expected["velZ"], 1e-5));
    }
}