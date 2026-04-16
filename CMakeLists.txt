cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ParticleSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(simulator
        main.cpp
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
)

find_package(ixwebsocket REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

target_include_directories(simulator PRIVATE
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(simulator PRIVATE
        ixwebsocket::ixwebsocket
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)
