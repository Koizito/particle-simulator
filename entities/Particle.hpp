#include <vector>
#include "Force.hpp"

struct Particle {
    float mass = 0; // grams
    float x = 0; // meters
    float y = 0; // meters
    float z = 0; // meters
    float vel_x = 0; // meters/second
    float vel_y = 0; // meters/second
    float vel_z = 0; // meters/second
    std::vector<Force> forces;
};