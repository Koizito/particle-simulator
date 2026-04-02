cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ParticleSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(simulator
    main.cpp
    entities/World.cpp
    entities/Particle.cpp
    entities/Force.cpp
    entities/OutgoingMessage.cpp
)

find_package(ixwebsocket REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(simulator PRIVATE
    ixwebsocket::ixwebsocket
    Threads::Threads
    nlohmann_json::nlohmann_json
)
