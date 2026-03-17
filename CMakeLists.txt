cmake_minimum_required(VERSION 3.16)

project(ParticleSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(simulator
    main.cpp
    entities/World.cpp
)

find_package(IXWebSocket CONFIG REQUIRED)

target_link_libraries(simulator PRIVATE ixwebsocket::client)