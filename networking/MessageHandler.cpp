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

                std::string type = jsonMessage.value("type", "");

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

                    World newWorld = parseWorldFromJson(jsonMessage);

                    updateWorld(newWorld);
                
                } else if (type == "reset_world") {
                    World newWorld;
                    {
                        std::lock_guard<std::mutex> resetWorldLock(appCtx.worldMutex);
                        appCtx.mainWorld = std::move(newWorld);
                    }
                } else if (type == "get_world_snapshot") {

                    World snapshotWorld = getWorldSnapshot();

                    pushWorldSnapshotToQueue(snapshotWorld);


                }
                else {
                    std::cout << "Unknown message type: " << type << "\n";
                    return;
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
    std::lock_guard<std::mutex> createParticleLock(appCtx.worldMutex);
    for (const auto& particleJson: particlesJson) {
        if (!appCtx.mainWorld.addParticle(particleJson, appCtx)) {
            std::cout << "Invalid particle creation\n";
            continue;
        }
    }
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

void MessageHandler::pushWorldSnapshotToQueue(const World& snapshotWorld) {
    nlohmann::json snapshotJson;
    snapshotJson["snapshot"] = snapshotWorld;
    {
        std::lock_guard<std::mutex> pushWorldSnapshotLock(appCtx.sendThreadMutex);
        appCtx.highPrioritySendQueue.emplace(snapshotJson.dump());
    }
    appCtx.checkIfSendThreadShouldRun.notify_one();
}
                    