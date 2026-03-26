#include <atomic>
#include <cstddef>
#include "entities/World.hpp"
#include "entities/OutgoingMessage.hpp"
#include <IXWebSocketServer.h>
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

bool areAllJsonFieldsValid(nlohmann::json json_message, std::vector<std::string> fieldsToCheck)
{
    for (auto& field : fieldsToCheck) {
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
        [&mainWorld](std::shared_ptr<ix::ConnectionState>,
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
                            std::lock_guard<std::mutex> lock(shouldRunMutex);
                            shouldRun = true;
                        }
                        shouldRunCV.notify_one();

                    } else if (type == "stop") {

                        {
                            std::lock_guard<std::mutex> lock(shouldRunMutex);
                            shouldRun = false;
                        }
                        shouldRunCV.notify_one();

                    } else if (type == "exit") {
                        {
                            std::lock_guard<std::mutex> lock(shouldRunMutex);
                            shouldRun = false;
                            shouldExit = true;
                        }
                        shouldRunCV.notify_one();

                    } else if (type == "create_particle") {

                        Particle newParticle(
                            json_message.value("mass", 1.0f),
                            json_message.value("x", 0.0f),
                            json_message.value("y", 0.0f),
                            json_message.value("z", 0.0f),
                            json_message.value("vel_x", 0.0f),
                            json_message.value("vel_y", 0.0f),
                            json_message.value("vel_z", 0.0f)
                        );

                        if (!isValidParticle(newParticle, mainWorld)){
                            std::cout << "Invalid particle creation\n";
                            return;
                        }

                        mainWorld.particles.push_back(std::move(newParticle));

                    } else if (type == "update_world") {

                        World newWorld(
                            json_message.value("max_x", 1.0f),
                            json_message.value("max_y", 1.0f),
                            json_message.value("max_z", 1.0f),
                            json_message.value("gravity_accel", -9.81f)
                        );

                        if (!isValidWorld(newWorld)) {
                            std::cout << "Invalid world update\n";
                            return;
                        }
                        
                        std::swap(mainWorld, newWorld);

                    } else if (type == "reset_world") {

                        World newWorld;
                        std::swap(mainWorld, newWorld);
                        
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
                if (message.binary)
                    client->sendBinary(message.data);
                else
                    client->send(message.data);
            }
        }
    });

    std::thread simThread([&mainWorld]()
    {    std::cout << "Starting the Particle Simulator \n\n";

        while (true) {

            {
                std::unique_lock<std::mutex> lock(shouldRunMutex);
                shouldRunCV.wait(lock, [] {
                    return shouldRun || shouldExit;
                });
            }
            
            if (shouldExit) break;

            std::this_thread::sleep_for(std::chrono::milliseconds(int(dt*1000)));

            std::vector<Particle> particles_snapshot;
            
            {
                std::lock_guard<std::mutex> lock(worldMutex);
                mainWorld.step(dt);
                particles_snapshot = mainWorld.particles;
            }

            size_t count = particles_snapshot.size();

            nlohmann::json header;
            header["type"] = "particles";
            header["count"] = count;

            std::vector<float> buffer;
            buffer.reserve(count * 7);

            for (const auto& p : particles_snapshot)
            {   
                buffer.push_back(p.mass);
                buffer.push_back(p.x);
                buffer.push_back(p.y);
                buffer.push_back(p.z);
                buffer.push_back(p.vel_x);
                buffer.push_back(p.vel_y);
                buffer.push_back(p.vel_z);
            }
            
            {
                std::lock_guard<std::mutex> lock(sendMutex);
                sendQueue.push(OutgoingMessage(false, header.dump()));
                sendQueue.push(OutgoingMessage(true, std::string(reinterpret_cast<char*>(buffer.data()), buffer.size() * sizeof(float))));
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