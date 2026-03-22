#include <cstddef>
#include "entities/World.hpp"
#include <IXWebSocketServer.h>
#include <IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <thread>

// Single client pointer
ix::WebSocket* currentClient = nullptr;

// Send data to client if connected
void sendToClient(const std::string& message)
{
    if (currentClient)
    {
        currentClient->send(message);
    }
}

int main()
{
    ix::initNetSystem();
    ix::WebSocketServer server(8080, "0.0.0.0");

    server.setOnClientMessageCallback(
        [](std::shared_ptr<ix::ConnectionState>,
           ix::WebSocket& webSocket,
           const std::unique_ptr<ix::WebSocketMessage>& msg)
        {
            if (msg->type == ix::WebSocketMessageType::Open)
            {
                if (currentClient != nullptr)
                {
                    webSocket.send("Server busy");
                    webSocket.close();
                    return;
                }
                std::cout << "Client connected\n";
                currentClient = &webSocket;
            }
            else if (msg->type == ix::WebSocketMessageType::Close)
            {
                std::cout << "Client disconnected\n";
                if (currentClient == &webSocket)
                    currentClient = nullptr;
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

    std::thread simThread([]()
    {    std::cout << "Starting the Particle Simulator \n\n";

        World mainWorld(100, 100, 100);

        mainWorld.particles.emplace_back(10.0f, 1.0f, 2.0f, 3.0f, 5.0f, 10.0f, 15.0f);
        mainWorld.particles.emplace_back(20.0f, 2.0f, 4.0f, 6.0f, 10.0f, 15.0f, 20.0f);

        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            mainWorld.step(0.1);
            
            size_t count = mainWorld.particles.size();

            nlohmann::json header;
            header["type"] = "particles";
            header["count"] = count;

            std::vector<float> buffer;
            buffer.reserve(count * 7);

            for (const auto& p : mainWorld.particles)
            {   
                buffer.push_back(p.mass);
                buffer.push_back(p.x);
                buffer.push_back(p.y);
                buffer.push_back(p.z);
                buffer.push_back(p.vel_x);
                buffer.push_back(p.vel_y);
                buffer.push_back(p.vel_z);
            }
            
            if (currentClient) {
                currentClient->send(header.dump());
                currentClient->sendBinary(
                    std::string(reinterpret_cast<char*>(buffer.data()),
                                buffer.size() * sizeof(float))
                );
            }
        }
    });

    simThread.join();

    server.stop();
    ix::uninitNetSystem();
}