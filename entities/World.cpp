#include "World.hpp"

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

        particle.x = particle.x + position_displacement(dt, particle.vel_x, accel_x);
        particle.y = particle.y + position_displacement(dt, particle.vel_y, accel_y);
        particle.z = particle.z + position_displacement(dt, particle.vel_z, accel_z);

        particle.vel_x = particle.vel_x + velocity_displacement(dt, accel_x);
        particle.vel_y = particle.vel_y + velocity_displacement(dt, accel_y);
        particle.vel_z = particle.vel_z + velocity_displacement(dt, accel_z);

        process_collision(particle);
    }
}

float World::position_displacement(float  dt, float vel, float accel) {
    return vel * dt + (accel * dt * dt) / 2;
}

float World::velocity_displacement(float  dt, float accel) {
    return accel * dt;
}

float World::acceleration(float force, float mass) {
    return force / (mass / 1000); // 1000 because mass is in grams.
}

void World::process_collision(Particle& particle) {
    if (particle.x > max_x) {
        particle.x = 2 * max_x - particle.x;
        particle.vel_x = -particle.vel_x;
    } else if (particle.x < 0) {
        particle.x = -particle.x; 
        particle.vel_x = -particle.vel_x;
    }

    if (particle.y > max_y) {
        particle.y = 2 * max_y - particle.y;
        particle.vel_y = -particle.vel_y;
    } else if (particle.y < 0) {
        particle.y = -particle.y;
        particle.vel_y = -particle.vel_y;
    }

    if (particle.z > max_z) {
        particle.z = 2 * max_z - particle.z; 
        particle.vel_z = -particle.vel_z;
    } else if (particle.z < 0) {
        particle.z = -particle.z; 
        particle.vel_z = -particle.vel_z;
    }
}