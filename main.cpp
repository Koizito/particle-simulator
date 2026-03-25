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

std::vector<std::string> particleRequiredFields = {
    "mass", "x", "y", "z", "vel_x", "vel_y", "vel_z"
};

std::vector<std::string> worldRequiredFieldsBoundaries = {
    "max_x", "max_y", "max_z"
};

std::vector<std::string> worldRequiredFieldsGravity = {
    "max_x", "max_y", "max_z"
};

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

                        if (!areAllJsonFieldsValid(json_message, particleRequiredFields)) return;

                        mainWorld.particles.emplace_back(
                            json_message["mass"],
                            json_message["x"],
                            json_message["y"],
                            json_message["z"],
                            json_message["vel_x"],
                            json_message["vel_y"],
                            json_message["vel_z"]
                        );

                    } else if (type == "change_world_size") {

                        if (!areAllJsonFieldsValid(json_message, worldRequiredFieldsBoundaries)) return;

                        mainWorld.max_x = json_message["max_x"];
                        mainWorld.max_y = json_message["max_y"];
                        mainWorld.max_z = json_message["max_z"];
                        
                    }  else if (type == "change_world_gravity") {

                        if (!areAllJsonFieldsValid(json_message, worldRequiredFieldsGravity)) return;

                        mainWorld.gravity_accel = json_message["gravity_accel"];

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

    simThread.join();
    sendThread.join();

    server.stop();
    ix::uninitNetSystem();
}