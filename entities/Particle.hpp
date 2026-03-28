#include <vector>
#include "Force.hpp"

struct Particle {
    int id;
    float mass; // grams
    float x; // meters
    float y; // meters
    float z; // meters
    float vel_x; // meters/second
    float vel_y; // meters/second
    float vel_z; // meters/second
    std::vector<Force> forces;

    Particle(int input_id, float input_mass=1.0f, float input_x=0.0f, float input_y=0.0f, float input_z=0.0f, float input_vel_x=0.0f, float input_vel_y=0.0f, float input_vel_z=0.0f);
};