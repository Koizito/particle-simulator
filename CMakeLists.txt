cmake_minimum_required(VERSION 3.16)

project(ParticleSimulator)

set(CMAKE_CXX_STANDARD 17)

add_executable(simulator
    main.cpp
    entities/World.cpp
)

target_include_directories(simulator PRIVATE include)