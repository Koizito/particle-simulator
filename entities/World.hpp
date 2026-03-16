#include <vector>
#include "Particle.hpp"

struct World {
    // Assume the minimum coordinates accepted are 0.0
    float maxX = 1.0;
    float maxY = 1.0;
    float maxZ = 1.0;
    
    std::vector<Particle> particles;

    void step(float  dt, float gravity_accel);
    float position_displacement(float  dt, float vel, float accel);
    float acceleration(float force, float mass);
    float velocity_displacement(float  dt, float accel);
    void process_collision(Particle& particle);
};