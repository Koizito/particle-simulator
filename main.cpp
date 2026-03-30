#include <atomic>
#include <cstddef>
#include "entities/World.hpp"
#include "entities/OutgoingMessage.hpp"
#include <IXWebSocketServer.h>
#include <IXWebSocketSendData.h>
#include <IXWebSocket.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <vector>

// Single client pointer
std::atomic<ix::WebSocket*> currentClient{nullptr};
// Default step for simulation
float dt = 1.0f;
// Counter for the particles ID assignment
std::atomic<int> particleIdCounter = 1;
// Should the continuous simulation be running
std::atomic<bool> shouldRun = false;
// Should the continuous simulation be exited
std::atomic<bool> shouldExit = false;
// Mutex to control the mainWorld access
std::mutex worldMutex;
// Mutex and condition variable to control the simulation thread
std::mutex shouldRunMutex;
std::condition_variable shouldRunCV;

std::mutex sendMutex;
std::queue<OutgoingMessage> sendQueue;
std::condition_variable sendCV;

constexpr int MAX_STEPS_PER_FRAME = 10;

const std::vector<std::string> particleRequiredFields = {
    "mass", "x", "y", "z", "vel_x", "vel_y", "vel_z"
};

const std::vector<std::string> worldRequiredFields = {
    "max_x", "max_y", "max_z", "gravity_accel"
};

bool areAllFieldsValid(const nlohmann::json& json_message, const std::vector<std::string>& fieldsToCheck)
{
    for (const auto& field : fieldsToCheck) {
        if (!json_message.contains(field)) {
            std::cerr << "Missing field: " << field << "\n";
            return false;
        }
    }
    return true;
}

bool isValidWorld(const World& newWorld)
{
    // dimension validations
    if (!std::isfinite(newWorld.max_x) || !std::isfinite(newWorld.max_y) || !std::isfinite(newWorld.max_z)) return false;
    if (newWorld.max_x < 1.0f || newWorld.max_y < 1.0f || newWorld.max_z < 1.0f) return false;
    if (newWorld.max_x > 1e6f || newWorld.max_y > 1e6f || newWorld.max_z > 1e6f) return false;

    // gravity acceleration validation
    if (!std::isfinite(newWorld.gravity_accel) || newWorld.gravity_accel > 1e6f) return false;

    return true;
}

bool isValidParticle(const Particle& particle, const World& world)
{
    // mass validations
    if (!std::isfinite(particle.mass) || particle.mass < 1e-6f || particle.mass > 1e6f) return false;

    // coordinates validations
    if (!std::isfinite(particle.x) || !std::isfinite(particle.y) || !std::isfinite(particle.z)) return false;
    if (particle.x < 0 || particle.x > world.max_x) return false;
    if (particle.y < 0 || particle.y > world.max_y) return false;
    if (particle.z < 0 || particle.z > world.max_z) return false;

    // velocity validations
    if (!std::isfinite(particle.vel_x) || !std::isfinite(particle.vel_y) || !std::isfinite(particle.vel_z)) return false;

    float maxDisplacementX = 10 * world.max_x;
    float maxDisplacementY = 10 * world.max_y;
    float maxDisplacementZ = 10 * world.max_z;

    float displacementX = std::abs(particle.vel_x * dt);
    float displacementY = std::abs(particle.vel_y * dt);
    float displacementZ = std::abs(particle.vel_z * dt);

    if (displacementX > maxDisplacementX || displacementY > maxDisplacementY || displacementZ > maxDisplacementZ) return false;

    return true;
}

int main()
{
    ix::initNetSystem();
    ix::WebSocketServer server(8080, "0.0.0.0");

    World mainWorld;

    server.setOnClientMessageCallback(
        [&mainWorld](const std::shared_ptr<ix::ConnectionState>&,
           ix::WebSocket& webSocket,
           const std::unique_ptr<ix::WebSocketMessage>& msg)        {
            if (msg->type == ix::WebSocketMessageType::Open) {

                ix::WebSocket* expected = nullptr;
                if (!currentClient.compare_exchange_strong(expected, &webSocket)) {
                    webSocket.send("Server busy");
                    webSocket.close();
                    return;
                }
                std::cout << "Client connected\n";

            } else if (msg->type == ix::WebSocketMessageType::Close) {

                ix::WebSocket* expected = &webSocket;
                currentClient.compare_exchange_strong(expected, nullptr);

                std::cout << "Client disconnected\n";

            } else if (msg->type == ix::WebSocketMessageType::Message) {

                nlohmann::json json_message;

                try {
                    json_message = nlohmann::json::parse(msg->str);
                } catch (...) {
                    std::cout << "Problem parsing the JSON!";
                    return;
                }
                
                std::string type = json_message.value("type", "");

                if (type.empty()) {
                    std::cout << "Missing 'type'\n";
                    return;
                }

                {
                    std::lock_guard<std::mutex> lock(worldMutex);

                    if (type == "start") {

                        {
                            std::lock_guard<std::mutex> startLock(shouldRunMutex);
                            shouldRun = true;
                        }
                        shouldRunCV.notify_one();

                    } else if (type == "stop") {

                        {
                            std::lock_guard<std::mutex> stopLock(shouldRunMutex);
                            shouldRun = false;
                        }
                        shouldRunCV.notify_one();

                    } else if (type == "exit") {
                        {
                            std::lock_guard<std::mutex> exitLock(shouldRunMutex);
                            shouldRun = false;
                            shouldExit = true;
                        }
                        shouldRunCV.notify_one();

                    } else if (type == "create_particle") {
                        
                        if (!json_message.contains("particles") || !json_message["particles"].is_array()) {
                            std::cout << "Missing particles array\n";
                            return;
                        }
                        
                        std::lock_guard<std::mutex> createParticleLock(worldMutex);
                        for (const auto& particle_json : json_message["particles"]) {
                            if (!areAllFieldsValid(particle_json, particleRequiredFields)) {
                                std::cout << "Missing particle fields\n";
                                continue;
                            }

                            Particle newParticle(
                                particleIdCounter.fetch_add(1),
                                particle_json["mass"],
                                particle_json["x"],
                                particle_json["y"],
                                particle_json["z"],
                                particle_json["vel_x"],
                                particle_json["vel_y"],
                                particle_json["vel_z"]
                            );

                            if (!isValidParticle(newParticle, mainWorld)){
                                std::cout << "Invalid particle creation\n";
                                continue;
                            }

                            mainWorld.particles.push_back(std::move(newParticle));
                        }

                    } else if (type == "delete_particle") {    

                        if (!json_message.contains("particle_ids") || !json_message["particle_ids"].is_array()) {
                            std::cout << "Missing particle IDs array\n";
                            return;
                        }

                        std::vector<int> ids = json_message["particle_ids"].get<std::vector<int>>();
                        std::unordered_set<int> idsToDelete(ids.begin(), ids.end());

                        std::lock_guard<std::mutex> deleteParticleLock(worldMutex);
                        mainWorld.deleteParticleById(idsToDelete);

                    } else if (type == "update_world") {

                        if (!areAllFieldsValid(json_message, worldRequiredFields)) {
                            std::cout << "Missing world fields\n";
                            return;
                        }
                        
                        World newWorld(
                            json_message["max_x"],
                            json_message["max_y"],
                            json_message["max_z"],
                            json_message["gravity_accel"]
                        );

                        if (!isValidWorld(newWorld)) {
                            std::cout << "Invalid world update\n";
                            return;
                        }
                        
                        {
                            std::lock_guard<std::mutex> updateWorldLock(worldMutex);
                            std::swap(mainWorld, newWorld);
                        }

                    } else if (type == "reset_world") {

                    World newWorld;
                    std::lock_guard<std::mutex> resetWorldLock(worldMutex);
                    std::swap(mainWorld, newWorld);

                    } else if (type == "get_world_snapshot") {

                        World snapshotWorld;
                        {
                            std::lock_guard<std::mutex> getWorldSnapshotLock(worldMutex);
                            snapshotWorld = mainWorld;
                        }

                        {
                            nlohmann::json snapshot = snapshotWorld;
                            std::lock_guard<std::mutex> pushWorldSnapshotLock(sendMutex);
                            sendQueue.emplace(snapshot.dump());
                        }
                    }
                }

            }
        }
    );

    if (!server.listen().first)
    {
        std::cerr << "Listen error\n";
        return 1;
    }

    server.start();
    std::cout << "Continuous simulation server running on port 8080\n";
    
    std::thread sendThread([]() {

        while (true) {

            std::unique_lock<std::mutex> lock(sendMutex);

            sendCV.wait(lock, [] {
                return !sendQueue.empty() || shouldExit;
            });

            if (shouldExit) break;

            auto message = sendQueue.front();
            sendQueue.pop();

            lock.unlock();

            if (auto client = currentClient.load(); client) {
                if (message.binary) {
                    ix::IXWebSocketSendData data(message.binaryData);
                    client->sendBinary(data);
                } else {
                    client->send(message.textData);
                }
            }
        }
    });

    std::thread simThread([&mainWorld]()
    {    std::cout << "Starting the Particle Simulator \n\n";

        using clock = std::chrono::steady_clock;
        auto next = clock::now();

        while (true) {

            {
                std::unique_lock<std::mutex> shouldRunLock(shouldRunMutex);
                shouldRunCV.wait(shouldRunLock, [] {
                    return shouldRun || shouldExit;
                });
            }
            
            if (shouldExit) break;

            int steps = 0;
            auto now = clock::now();

            while (now >= next && steps < MAX_STEPS_PER_FRAME) {

                {
                    std::lock_guard<std::mutex> stepLock(worldMutex);
                    mainWorld.step(dt);
                }

                next += std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(dt));
                steps++;
            }

            if (steps >= MAX_STEPS_PER_FRAME) {
                next = clock::now();
            }

            // Snapshot AFTER stepping (only once per frame)
            std::vector<Particle> particles_snapshot;
            {
                std::lock_guard<std::mutex> lock(worldMutex);
                particles_snapshot = mainWorld.particles;
            }

            size_t count = particles_snapshot.size();

            nlohmann::json metadata;
            metadata["type"] = "particles";
            metadata["count"] = count;
            
            std::vector<uint8_t> buffer_bytes;
            buffer_bytes.reserve(count * sizeof(int) + count * 7 * sizeof(float));

            for (const auto& p : particles_snapshot) {
                // ID
                auto id_ptr = reinterpret_cast<const uint8_t*>(&p.id);
                buffer_bytes.insert(buffer_bytes.end(), id_ptr, id_ptr + sizeof(int));

                // mass, position, velocity
                const float arr[] = {p.mass, p.x, p.y, p.z, p.vel_x, p.vel_y, p.vel_z};
                auto arr_ptr = reinterpret_cast<const uint8_t*>(arr);
                buffer_bytes.insert(buffer_bytes.end(), arr_ptr, arr_ptr + sizeof(arr));
            }

            {
                std::lock_guard<std::mutex> lock(sendMutex);
                sendQueue.emplace(metadata.dump());
                sendQueue.emplace(std::move(buffer_bytes));
            }
            sendCV.notify_one();
        }
    });
    
    simThread.join();
    sendCV.notify_all();
    sendThread.join();

    server.stop();
    ix::uninitNetSystem();
}