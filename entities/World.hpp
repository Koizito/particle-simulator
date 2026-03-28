#include <unordered_set>
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
    void position_velocity_calculation(float  dt, float& pos, float& vel, float accel, float max);
    float acceleration(float force, float mass);
    void deleteParticleById(std::unordered_set<int>& idsToDelete);
};