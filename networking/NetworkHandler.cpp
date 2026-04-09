#include "NetworkingHandler.hpp"

NetworkingHandler::NetworkingHandler(AppContext& inputAppCtx)
    : appCtx(inputAppCtx), server(8080, "0.0.0.0") {
}

bool NetworkingHandler::startServer() {
    ix::initNetSystem();
    server.setOnClientMessageCallback(
        [this](const std::shared_ptr<ix::ConnectionState>&,
               ix::WebSocket& webSocket,
               const std::unique_ptr<ix::WebSocketMessage>& msg) {
            if (msg->type == ix::WebSocketMessageType::Open) {
                ix::WebSocket* expected = nullptr;
                if (!this->appCtx.currentClient.compare_exchange_strong(expected, &webSocket)) {
                    webSocket.send("Server busy");
                    webSocket.close();
                    return;
                }
                std::cout << "Client connected\n";
            } else if (msg->type == ix::WebSocketMessageType::Close) {
                ix::WebSocket* expected = &webSocket;
                this->appCtx.currentClient.compare_exchange_strong(expected, nullptr);

                std::cout << "Client disconnected\n";
            } else if (msg->type == ix::WebSocketMessageType::Message) {
                nlohmann::json jsonMessage;

                try {
                    jsonMessage = nlohmann::json::parse(msg->str);
                } catch (...) {
                    std::cout << "Problem parsing the JSON!\n";
                    return;
                }

                std::string type = jsonMessage.value("type", "");

                if (type.empty()) {
                    std::cout << "Missing 'type'\n";
                    return;
                }

                if (type == "start") {
                    this->appCtx.shouldSimulationThreadRun.store(true);
                    this->appCtx.shouldSendThreadRun.store(true);

                    this->appCtx.checkIfSimulationThreadShouldRun.notify_one();
                    this->appCtx.checkIfSendThreadShouldRun.notify_one();
                } else if (type == "stop") {
                    this->appCtx.shouldSimulationThreadRun.store(false);
                    this->appCtx.shouldSendThreadRun.store(false);

                    this->appCtx.checkIfSimulationThreadShouldRun.notify_one();
                    this->appCtx.checkIfSendThreadShouldRun.notify_one();
                } else if (type == "exit") {
                    this->appCtx.signalExit();
                } else if (type == "create_particles") {
                    if (!jsonMessage.contains("particles") || !jsonMessage["particles"].is_array()) {
                        std::cout << "Missing particles array\n";
                        return;
                    }

                    std::lock_guard<std::mutex> createParticleLock(this->appCtx.worldMutex);
                    for (const auto& particleJson: jsonMessage["particles"]) {
                        if (!this->appCtx.mainWorld.addParticle(particleJson)) {
                            std::cout << "Invalid particle creation\n";
                            continue;
                        }
                    }
                } else if (type == "delete_particles") {
                    if (!jsonMessage.contains("particle_ids") || !jsonMessage["particle_ids"].is_array()) {
                        std::cout << "Missing particle IDs array\n";
                        return;
                    }

                    std::vector<int> ids = jsonMessage["particle_ids"].get<std::vector<int> >();
                    std::unordered_set<int> idsToDelete(ids.begin(), ids.end());

                    std::lock_guard<std::mutex> deleteParticleLock(this->appCtx.worldMutex);
                    this->appCtx.mainWorld.deleteParticleById(idsToDelete);
                } else if (type == "update_world") {
                    double dt_ms = jsonMessage.value("dt", this->appCtx.mainWorld.dt * 1000);
                    double dt_s = dt_ms / 1000;
                    World newWorld(
                        jsonMessage.value("maxX", this->appCtx.mainWorld.maxX),
                        jsonMessage.value("maxY", this->appCtx.mainWorld.maxY),
                        jsonMessage.value("maxZ", this->appCtx.mainWorld.maxZ),
                        static_cast<float>(dt_s),
                        jsonMessage.value("gravityAccel", this->appCtx.mainWorld.gravityAccel)
                    );

                    if (!newWorld.isValid()) {
                        std::cout << "Invalid world update\n";
                        return;
                    }

                    {
                        std::lock_guard<std::mutex> updateWorldLock(this->appCtx.worldMutex);
                        this->appCtx.mainWorld.maxX = newWorld.maxX;
                        this->appCtx.mainWorld.maxY = newWorld.maxY;
                        this->appCtx.mainWorld.maxZ = newWorld.maxZ;
                        this->appCtx.mainWorld.dt = newWorld.dt;
                        this->appCtx.mainWorld.gravityAccel = newWorld.gravityAccel;

                        this->appCtx.mainWorld.particles = std::move(newWorld.particles);
                        this->appCtx.mainWorld.particleIdCounter = newWorld.particleIdCounter.load();
                    }
                } else if (type == "reset_world") {
                    World newWorld;
                    std::lock_guard<std::mutex> resetWorldLock(this->appCtx.worldMutex);
                    this->appCtx.mainWorld.maxX = newWorld.maxX;
                    this->appCtx.mainWorld.maxY = newWorld.maxY;
                    this->appCtx.mainWorld.maxZ = newWorld.maxZ;
                    this->appCtx.mainWorld.dt = newWorld.dt;
                    this->appCtx.mainWorld.gravityAccel = newWorld.gravityAccel;

                    this->appCtx.mainWorld.particles = std::move(newWorld.particles);
                    this->appCtx.mainWorld.particleIdCounter = newWorld.particleIdCounter.load();
                } else if (type == "get_world_snapshot") {
                    World snapshotWorld;
                    {
                        std::lock_guard<std::mutex> getWorldSnapshotLock(this->appCtx.worldMutex);
                        this->appCtx.mainWorld.fillSnapshot(snapshotWorld);
                    }

                    {
                        nlohmann::json snapshotJson;
                        snapshotJson["snapshot"] = snapshotWorld;
                        std::lock_guard<std::mutex> pushWorldSnapshotLock(this->appCtx.sendThreadMutex);
                        this->appCtx.highPrioritySendQueue.emplace(snapshotJson.dump());
                    }
                    this->appCtx.checkIfSendThreadShouldRun.notify_one();
                }
            }
        }
    );

    auto result = server.listen();
    if (!result.first) {
        std::cerr << "Listen error: " << result.second << "\n";
        return false;
    }

    server.start();
    std::cout << "Continuous simulation server running on port 8080\n";
    return true;
}

void NetworkingHandler::stopServer() {
    server.stop();
    ix::uninitNetSystem();
}
