#include <iostream>
#include "entities/World.hpp"

int main() {
    std::cout << "Starting the Particle Simulator \n\n";

    World mainWorld;
    mainWorld.maxX = 100;
    mainWorld.maxY = 100;
    mainWorld.maxZ = 100;

    Particle particleA;
    particleA.mass = 10;
    particleA.x = 1;
    particleA.y = 2;
    particleA.z = 3;
    particleA.vel_x = 5;
    particleA.vel_y = 5;
    particleA.vel_z = 5;

    Particle particleB;
    particleB.mass = 20;
    particleB.x = 2;
    particleB.y = 4;
    particleB.z = 6;
    particleB.vel_x = 10;
    particleB.vel_y = 10;
    particleB.vel_z = 10;

    mainWorld.particles.push_back(particleA);
    mainWorld.particles.push_back(particleB);

    float gravity_accel = -9.81;

    for (int i = 0; i < mainWorld.particles.size(); i++) {
        Particle& particle = mainWorld.particles[i];
        std::cout << "Particle: " << i+1 << "\n";
        std::cout << "Mass: " << particle.mass << "\n";
        std::cout << "X coordinate: " << particle.x << "\n";
        std::cout << "Y coordinate: " << particle.y << "\n";
        std::cout << "Z coordinate: " << particle.z << "\n";
        std::cout << "Velocity in X coordinate: " << particle.vel_x << "\n";
        std::cout << "Velocity in Y coordinate: " << particle.vel_y << "\n";
        std::cout << "Velocity in Z coordinate: " << particle.vel_z << "\n\n";
    }

    std::cout << "Running simulation... \n\n";

    float current_time = 0.0;
    float time_step = 0.5;

    while (current_time <= 10.0) {
        for (int i = 0; i < mainWorld.particles.size(); i++) {
            Particle& particle = mainWorld.particles[i];
            std::cout << "Particle: " << i+1 << "\n";
            std::cout << "X coordinate: " << particle.x << "\n";
            std::cout << "Y coordinate: " << particle.y << "\n";
            std::cout << "Z coordinate: " << particle.z << "\n\n";
        }

        mainWorld.step(time_step, gravity_accel);
        
        current_time = current_time + time_step;

    }
}