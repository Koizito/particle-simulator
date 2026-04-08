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
                nlohmann::json json_message;

                try {
                    json_message = nlohmann::json::parse(msg->str);
                } catch (...) {
                    std::cout << "Problem parsing the JSON!\n";
                    return;
                }

                std::string type = json_message.value("type", "");

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
                    if (!json_message.contains("particles") || !json_message["particles"].is_array()) {
                        std::cout << "Missing particles array\n";
                        return;
                    }

                    std::lock_guard<std::mutex> createParticleLock(this->appCtx.worldMutex);
                    for (const auto& particleJson: json_message["particles"]) {
                        if (!this->appCtx.mainWorld.addParticle(particleJson)) {
                            std::cout << "Invalid particle creation\n";
                            continue;
                        }
                    }
                } else if (type == "delete_particles") {
                    if (!json_message.contains("particle_ids") || !json_message["particle_ids"].is_array()) {
                        std::cout << "Missing particle IDs array\n";
                        return;
                    }

                    std::vector<int> ids = json_message["particle_ids"].get<std::vector<int> >();
                    std::unordered_set<int> idsToDelete(ids.begin(), ids.end());

                    std::lock_guard<std::mutex> deleteParticleLock(this->appCtx.worldMutex);
                    this->appCtx.mainWorld.deleteParticleById(idsToDelete);
                } else if (type == "update_world") {
                    double dt_ms = json_message.value("dt", this->appCtx.mainWorld.dt * 1000);
                    double dt_s = dt_ms / 1000;
                    World newWorld(
                        json_message.value("max_x", this->appCtx.mainWorld.max_x),
                        json_message.value("max_y", this->appCtx.mainWorld.max_y),
                        json_message.value("max_z", this->appCtx.mainWorld.max_z),
                        static_cast<float>(dt_s),
                        json_message.value("gravity_accel", this->appCtx.mainWorld.gravity_accel)
                    );

                    if (!newWorld.isValid()) {
                        std::cout << "Invalid world update\n";
                        return;
                    }

                    {
                        std::lock_guard<std::mutex> updateWorldLock(this->appCtx.worldMutex);
                        this->appCtx.mainWorld.max_x = newWorld.max_x;
                        this->appCtx.mainWorld.max_y = newWorld.max_y;
                        this->appCtx.mainWorld.max_z = newWorld.max_z;
                        this->appCtx.mainWorld.dt = newWorld.dt;
                        this->appCtx.mainWorld.gravity_accel = newWorld.gravity_accel;

                        this->appCtx.mainWorld.particles = std::move(newWorld.particles);
                        this->appCtx.mainWorld.particleIdCounter = newWorld.particleIdCounter.load();
                    }
                } else if (type == "reset_world") {
                    World newWorld;
                    std::lock_guard<std::mutex> resetWorldLock(this->appCtx.worldMutex);
                    this->appCtx.mainWorld.max_x = newWorld.max_x;
                    this->appCtx.mainWorld.max_y = newWorld.max_y;
                    this->appCtx.mainWorld.max_z = newWorld.max_z;
                    this->appCtx.mainWorld.dt = newWorld.dt;
                    this->appCtx.mainWorld.gravity_accel = newWorld.gravity_accel;

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
