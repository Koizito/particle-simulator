cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ParticleSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(simulator_lib STATIC
        core/World.cpp
        core/Particle.cpp
        core/Force.cpp
        core/OutgoingMessage.cpp
        networking/NetworkingHandler.cpp
        networking/MessageHandler.cpp
        app/AppContext.cpp
        threads/BaseThread.cpp
        threads/SimulationThread.cpp
        threads/SendThread.cpp
        app/MessagingQueue.cpp
)

target_include_directories(simulator_lib PUBLIC ${CMAKE_SOURCE_DIR})

find_package(ixwebsocket REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

target_link_libraries(simulator_lib PUBLIC
        ixwebsocket::ixwebsocket
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

add_executable(simulator main.cpp)
target_link_libraries(simulator PRIVATE simulator_lib)