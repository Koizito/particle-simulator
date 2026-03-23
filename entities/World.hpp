#include <vector>
#include "Particle.hpp"

struct World {
    // Assume the minimum coordinates are always 0.0
    float max_x;
    float max_y;
    float max_z;

    float gravity_accel;
    
    std::vector<Particle> particles;

    World(float input_max_x=1.0f, float input_max_y=1.0f, float input_max_z=1.0f, float input_gravity_accel=9.81f);

    void step(float  dt);
    float position_displacement(float  dt, float vel, float accel);
    float acceleration(float force, float mass);
    float velocity_displacement(float  dt, float accel);
    void process_collision(Particle& particle);
};