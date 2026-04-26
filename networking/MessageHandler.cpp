#include "MessageHandler.hpp"
#include "app/AppContext.hpp"
#include "IXWebSocketServer.h"
#include "core/OutgoingMessage.hpp"
#include "nlohmann/json.hpp"


MessageHandler::MessageHandler(std::shared_ptr<spdlog::logger> inputLogger, AppContext& inputAppCtx)
    : logger(std::move(inputLogger)), appCtx(inputAppCtx) {
}

void MessageHandler::handleMessage(const std::string& message) {
    logger->info("Processing message.");
    nlohmann::json jsonMessage = parseMessage(message);

    const std::string type = jsonMessage.value("type", "");

    if (type.empty()) {
        logger->error("Missing 'type' field");
        return;
    }

    if (type == "start") {
        logger->info("Starting simulation...");
        appCtx.setSimulationState(true, true);
    } else if (type == "stop") {
        logger->info("Stopping simulation...");
        appCtx.setSimulationState(false, false);
    } else if (type == "exit") {
        logger->info("Exiting simulation...");
        appCtx.signalExit();
    } else if (type == "create_particles") {
        if (!jsonMessage.contains("particles") || !jsonMessage["particles"].is_array()) {
            logger->error("Missing array of particles");
            return;
        }
        logger->info("Creating {} new particles", jsonMessage["particles"].size());
        logger->debug("Complete particles array: {}", jsonMessage["particles"].dump());
        addParticlesToWorld(jsonMessage["particles"]);
    } else if (type == "delete_particles") {
        if (!jsonMessage.contains("particle_ids") || !jsonMessage["particle_ids"].is_array()) {
            logger->error("Missing array of particle IDs");
            return;
        }

        logger->info("Deleting {} particles", jsonMessage["particle_ids"].size());
        logger->debug("Complete particle IDs array: {}", jsonMessage["particle_ids"].dump());

        std::unordered_set<int> idsToDelete = getParticleIdsFromJson(jsonMessage["particle_ids"]);

        std::lock_guard<std::mutex> deleteParticleLock(appCtx.worldMutex);
        appCtx.mainWorld.deleteParticlesById(idsToDelete);
    } else if (type == "update_world") {
        logger->info("Updating world...");
        logger->debug("Input world payload: {}", jsonMessage["world"].dump());
        const World newWorld = parseWorldFromJson(jsonMessage["world"]);

        updateWorld(newWorld);
    } else if (type == "reset_world") {
        logger->info("Resetting world...");
        World newWorld;
        std::lock_guard<std::mutex> resetWorldLock(appCtx.worldMutex);
        appCtx.mainWorld = std::move(newWorld);
    } else if (type == "get_world_snapshot") {
        logger->info("Getting world snapshot...");
        const World snapshotWorld = getWorldSnapshot();

        pushWorldSnapshotToQueue(snapshotWorld);
    } else {
        logger->error("Unknown type: {}", type);
    }
}

nlohmann::json MessageHandler::parseMessage(const std::string& message) {
    nlohmann::json jsonMessage;

    try {
        jsonMessage = nlohmann::json::parse(message);
    } catch (...) {
        logger->error("Failed to parse message: {}", message);
        return {};
    }

    return jsonMessage;
}

void MessageHandler::addParticlesToWorld(const nlohmann::json& particlesJson) {
    for (const auto& particleJson: particlesJson) {
        if (particleJsonHasMissingFields(particleJson) || particleJsonHasNonNumberFields(particleJson)) {
            logger->warn("Invalid particle detected in input");
            logger->debug("Invalid particle payload: {}", particleJson.dump());
            continue;
        }

        {
            Particle newParticle(
                appCtx.particleIdCounter.fetch_add(1),
                particleJson.at("mass"),
                particleJson.at("x"),
                particleJson.at("y"),
                particleJson.at("z"),
                particleJson.at("velX"),
                particleJson.at("velY"),
                particleJson.at("velZ")
            );
            std::lock_guard<std::mutex> createParticleLock(appCtx.worldMutex);
            if (!appCtx.mainWorld.addParticle(newParticle)) {
                logger->warn("Failed to add particle to world");
            }
        }
    }
}

bool MessageHandler::particleJsonHasNonNumberFields(const nlohmann::json& particleJson) {
    return (!particleJson["mass"].is_number() ||
            !particleJson["x"].is_number() ||
            !particleJson["y"].is_number() ||
            !particleJson["z"].is_number() ||
            !particleJson["velX"].is_number() ||
            !particleJson["velY"].is_number() ||
            !particleJson["velZ"].is_number());
}

bool MessageHandler::particleJsonHasMissingFields(const nlohmann::json& particleJson) {
    return (!particleJson.contains("mass") ||
            !particleJson.contains("x") ||
            !particleJson.contains("y") ||
            !particleJson.contains("z") ||
            !particleJson.contains("velX") ||
            !particleJson.contains("velY") ||
            !particleJson.contains("velZ"));
}

std::unordered_set<int> MessageHandler::getParticleIdsFromJson(const nlohmann::json& particleIdsJson) {
    std::unordered_set<int> idsToDelete;
    for (const auto& idJson: particleIdsJson) {
        if (!idJson.is_number_integer()) {
            logger->warn("Invalid particle ID detected in input: {}", idJson.dump());
            continue;
        }
        idsToDelete.insert(idJson.get<int>());
    }
    return idsToDelete;
}

World MessageHandler::parseWorldFromJson(const nlohmann::json& jsonMessage) {
    std::lock_guard<std::mutex> updateWorldLock(appCtx.worldMutex);
    double dt_ms = jsonMessage.value("dt", appCtx.mainWorld.dt * 1000);
    double dt_s = dt_ms / 1000;
    World newWorld(
        jsonMessage.value("maxX", appCtx.mainWorld.maxX),
        jsonMessage.value("maxY", appCtx.mainWorld.maxY),
        jsonMessage.value("maxZ", appCtx.mainWorld.maxZ),
        static_cast<float>(dt_s),
        jsonMessage.value("gravityAccel", appCtx.mainWorld.gravityAccel)
    );
    return newWorld;
}

void MessageHandler::updateWorld(World newWorld) {
    std::lock_guard<std::mutex> worldLock(appCtx.worldMutex);

    if (!newWorld.isValid() && appCtx.mainWorld.canUpdateBounds(newWorld.maxX, newWorld.maxY, newWorld.maxZ)) {
        logger->error("Invalid world detected");
        return;
    }

    newWorld.particles = std::move(appCtx.mainWorld.particles);
    appCtx.mainWorld = std::move(newWorld);
}

World MessageHandler::getWorldSnapshot() {
    World snapshotWorld;
    std::lock_guard<std::mutex> getWorldSnapshotLock(appCtx.worldMutex);
    appCtx.mainWorld.fillSnapshot(snapshotWorld);
    return snapshotWorld;
}

void MessageHandler::pushWorldSnapshotToQueue(const World& snapshotWorld) const {
    nlohmann::json snapshotJson;
    snapshotJson["snapshot"] = snapshotWorld;

    logger->debug("World snapshot to be pushed to queue: {}", snapshotJson.dump());

    appCtx.messagingQueue.pushMessageToHighPriorityQueue(OutgoingMessage(snapshotJson.dump()));
}
