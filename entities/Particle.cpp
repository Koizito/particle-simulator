#include "Particle.hpp"

Particle::Particle(float input_mass, float input_x, float input_y, float input_z, float input_vel_x, float input_vel_y, float input_vel_z)
    : mass(input_mass), x(input_x), y(input_y), z(input_z), vel_x(input_vel_x), vel_y(input_vel_y), vel_z(input_vel_z) {}