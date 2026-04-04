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
// Should the continuous simulation be running
std::atomic<bool> shouldRun = false;
// Should the send threah be running
std::atomic<bool> shouldSend = false;
// Should the continuous simulation be exited
std::atomic<bool> shouldExit = false;
// Mutex to control the mainWorld access
std::mutex worldMutex;
// Mutex and condition variable to control the simulation thread
std::mutex shouldRunMutex;
std::condition_variable shouldRunCV;

std::mutex sendMutex;
std::condition_variable shouldSendCV;
std::queue<OutgoingMessage> highPrioritySendQueue;
std::queue<OutgoingMessage> normalSendQueue;

constexpr int MAX_STEPS_PER_FRAME = 10;
constexpr size_t MAX_QUEUE_SIZE = 100;

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

                if (type == "start") {

                    {
                        std::lock_guard<std::mutex> startLock(shouldRunMutex);
                        shouldRun = true;
                        shouldSend = true;
                    }
                    shouldRunCV.notify_one();
                    shouldSendCV.notify_one();

                } else if (type == "stop") {

                    {
                        std::lock_guard<std::mutex> stopLock(shouldRunMutex);
                        shouldRun = false;
                        shouldSend = false;
                    }
                    shouldRunCV.notify_one();
                    shouldSendCV.notify_one();

                } else if (type == "exit") {
                    {
                        std::lock_guard<std::mutex> exitLock(shouldRunMutex);
                        shouldRun = false;
                        shouldSend = false;
                        shouldExit = true;
                    }
                    shouldRunCV.notify_one();
                    shouldSendCV.notify_one();

                } else if (type == "create_particle") {

                    if (!json_message.contains("particles") || !json_message["particles"].is_array()) {
                        std::cout << "Missing particles array\n";
                        return;
                    }

                    std::lock_guard<std::mutex> createParticleLock(worldMutex);
                    for (const auto& particleJson : json_message["particles"]) {
                        if (!mainWorld.addParticle(particleJson)) {
                            std::cout << "Invalid particle creation\n";
                            continue;
                        }
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
                    World newWorld(
                        json_message.value("max_x", mainWorld.max_x),
                        json_message.value("max_y", mainWorld.max_y),
                        json_message.value("max_z", mainWorld.max_z),
                        json_message.value("dt", mainWorld.dt),
                        json_message.value("gravity_accel", mainWorld.gravity_accel)
                    );

                    if (!isValidWorld(newWorld)) {
                        std::cout << "Invalid world update\n";
                        return;
                    }

                    {
                        std::lock_guard<std::mutex> updateWorldLock(worldMutex);
                        mainWorld.max_x = newWorld.max_x;
                        mainWorld.max_y = newWorld.max_y;
                        mainWorld.max_z = newWorld.max_z;
                        mainWorld.dt = newWorld.dt;
                        mainWorld.gravity_accel = newWorld.gravity_accel;

                        mainWorld.particles = std::move(newWorld.particles);
                        mainWorld.particleIdCounter = newWorld.particleIdCounter.load();
                    }

                } else if (type == "reset_world") {

                    World newWorld;
                    std::lock_guard<std::mutex> resetWorldLock(worldMutex);
                    mainWorld.max_x = newWorld.max_x;
                    mainWorld.max_y = newWorld.max_y;
                    mainWorld.max_z = newWorld.max_z;
                    mainWorld.dt = newWorld.dt;
                    mainWorld.gravity_accel = newWorld.gravity_accel;

                    mainWorld.particles = std::move(newWorld.particles);
                    mainWorld.particleIdCounter = newWorld.particleIdCounter.load();

                } else if (type == "get_world_snapshot") {

                    World snapshotWorld;
                    {
                        std::lock_guard<std::mutex> getWorldSnapshotLock(worldMutex);
                        mainWorld.fillSnapshot(snapshotWorld);
                    }

                    {
                        nlohmann::json snapshotJson;
                        snapshotJson["snapshot"] = snapshotWorld;
                        std::lock_guard<std::mutex> pushWorldSnapshotLock(sendMutex);
                        highPrioritySendQueue.emplace(snapshotJson.dump());
                    }
                    shouldSendCV.notify_one();
                    
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

            std::unique_lock<std::mutex> sendLock(sendMutex);

            shouldSendCV.wait(sendLock, [] {
                return (shouldSend && !normalSendQueue.empty()) || !highPrioritySendQueue.empty() || shouldExit;
            });

            if (shouldExit) break;

            OutgoingMessage message;

            if (!highPrioritySendQueue.empty()) {
                message = std::move(highPrioritySendQueue.front());
                highPrioritySendQueue.pop();
            } else {
                message = std::move(normalSendQueue.front());
                normalSendQueue.pop();
            }

            shouldRunCV.notify_one();
            sendLock.unlock();

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
                std::unique_lock<std::mutex> runLock(shouldRunMutex);
                shouldRunCV.wait(runLock, [&] {
                    return shouldRun || shouldExit;
                });
            }
            
            if (shouldExit) break;

            bool queueFull = false;
            {
                std::lock_guard<std::mutex> lock(sendMutex);
                queueFull = normalSendQueue.size() > MAX_QUEUE_SIZE;
            }

            if (queueFull) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            int steps = 0;
            auto now = clock::now();

            while (now >= next && steps < MAX_STEPS_PER_FRAME) {

                {
                    std::lock_guard<std::mutex> stepLock(worldMutex);
                    mainWorld.step();
                }

                next += std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(mainWorld.dt));
                steps++;
            }

            if (steps >= MAX_STEPS_PER_FRAME) {
                next = clock::now();
            }

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
                normalSendQueue.emplace(metadata.dump());
                normalSendQueue.emplace(std::move(buffer_bytes));
            }
            shouldSendCV.notify_one();
        }
    });
    
    simThread.join();
    shouldSendCV.notify_all();
    sendThread.join();

    server.stop();
    ix::uninitNetSystem();
}