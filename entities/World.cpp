#include "World.hpp"
#include <cmath>

World::World(float input_max_x, float input_max_y, float input_max_z, float input_gravity_accel)
    : max_x(input_max_x), max_y(input_max_y), max_z(input_max_z), gravity_accel(input_gravity_accel) {}

void World::step(float dt) {
    
    for (Particle& particle : particles) {
        float particle_force_x = 0;
        float particle_force_y = 0;
        float particle_force_z = 0;

        for (Force& force : particle.forces){
            particle_force_x = particle_force_x + force.x;
            particle_force_y = particle_force_y + force.y;
            particle_force_z = particle_force_z + force.z;
        }

        float accel_x = acceleration(particle_force_x, particle.mass);
        float accel_y = acceleration(particle_force_y, particle.mass);
        float accel_z = acceleration(particle_force_z, particle.mass) + gravity_accel;

        position_velocity_calculation(dt, particle.x, particle.vel_x, accel_x, max_x);
        position_velocity_calculation(dt, particle.y, particle.vel_y, accel_y, max_y);
        position_velocity_calculation(dt, particle.z, particle.vel_z, accel_z, max_z);
    }
}

void World::position_velocity_calculation(float  dt, float& pos, float& vel, float accel, float max) {
    float L = max;
    float period = 2*L;

    // compute displacement including acceleration
    float newPos = pos + vel*dt + 0.5f*accel*dt*dt;

    // wrap position into repeated mirrored space
    float posRemainder = std::fmod(newPos, period);
    if (posRemainder < 0) posRemainder += period;

    // decide reflection
    if (posRemainder > L) {
        pos = period - posRemainder;
        vel = -(vel + accel*dt); // velocity flips after timestep
    } else {
        pos = posRemainder;
        vel = vel + accel*dt; // standard velocity update
    }
}

float World::acceleration(float force, float mass) {
    return force / (mass / 1000); // 1000 because mass is in grams.
}

void World::deleteParticleById(std::unordered_set<int>& idsToDelete) {
    auto it = std::remove_if(particles.begin(), particles.end(),
                             [&idsToDelete](const Particle& p){ return idsToDelete.contains(p.id); });
    particles.erase(it, particles.end());
}