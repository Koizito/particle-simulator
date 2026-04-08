#include "Particle.hpp"

Particle::Particle(const int input_id, const float input_mass, const float input_x, const float input_y,
                   const float input_z, const float input_vel_x, const float input_vel_y, const float input_vel_z)
    : id(input_id), mass(input_mass), x(input_x), y(input_y), z(input_z), vel_x(input_vel_x), vel_y(input_vel_y),
      vel_z(input_vel_z) {
}
