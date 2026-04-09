#include "MessageHandler.hpp"
#include "app/AppContext.hpp"
#include "IXWebSocketServer.h"
#include "nlohmann/json.hpp"
#include <iostream>


MessageHandler::MessageHandler(AppContext& inputAppCtx)
    : appCtx(inputAppCtx) {
}

void MessageHandler::handleMessage(const std::string& message) {
    nlohmann::json jsonMessage = parseMessage(message);

    const std::string type = jsonMessage.value("type", "");

    if (type.empty()) {
        std::cout << "Missing 'type'\n";
        return;
    }

    if (type == "start") {
        appCtx.setSimulationState(true, true);
    } else if (type == "stop") {
        appCtx.setSimulationState(false, false);
    } else if (type == "exit") {
        appCtx.signalExit();
    } else if (type == "create_particles") {
        if (!jsonMessage.contains("particles") || !jsonMessage["particles"].is_array()) {
            std::cout << "Missing particles array\n";
            return;
        }

        addParticlesToWorld(jsonMessage["particles"]);
    } else if (type == "delete_particles") {
        if (!jsonMessage.contains("particle_ids") || !jsonMessage["particle_ids"].is_array()) {
            std::cout << "Missing particle IDs array\n";
            return;
        }

        std::unordered_set<int> idsToDelete = getParticleIdsFromJson(jsonMessage["particle_ids"]);

        std::lock_guard<std::mutex> deleteParticleLock(appCtx.worldMutex);
        appCtx.mainWorld.deleteParticlesById(idsToDelete);
    } else if (type == "update_world") {
        const World newWorld = parseWorldFromJson(jsonMessage);

        updateWorld(newWorld);
    } else if (type == "reset_world") {
        World newWorld;
        std::lock_guard<std::mutex> resetWorldLock(appCtx.worldMutex);
        appCtx.mainWorld = std::move(newWorld);
    } else if (type == "get_world_snapshot") {
        const World snapshotWorld = getWorldSnapshot();

        pushWorldSnapshotToQueue(snapshotWorld);
    } else {
        std::cout << "Unknown message type: " << type << "\n";
    }
}

nlohmann::json MessageHandler::parseMessage(const std::string& message) {
    nlohmann::json jsonMessage;

    try {
        jsonMessage = nlohmann::json::parse(message);
    } catch (...) {
        std::cout << "Problem parsing the JSON!\n";
        return {};
    }

    return jsonMessage;
}

void MessageHandler::addParticlesToWorld(const nlohmann::json& particlesJson) {
    for (const auto& particleJson: particlesJson) {
        if (particleJsonHasMissingFields(particleJson) || particleJsonHasNonNumberFields(particleJson)) {
            std::cout << "Invalid particle detected\n";
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
                std::cout << "Invalid particle detected\n";
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
            std::cout << "Invalid particle ID\n";
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
        std::cout << "Invalid world update\n";
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
    {
        std::lock_guard<std::mutex> pushWorldSnapshotLock(appCtx.sendThreadMutex);
        appCtx.highPrioritySendQueue.emplace(snapshotJson.dump());
    }
    appCtx.checkIfSendThreadShouldRun.notify_one();
}
