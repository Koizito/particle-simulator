#include "core/Particle.hpp"

Particle::Particle(const int inputId, const float inputMass, const float inputX, const float inputY,
                   const float inputZ, const float inputVelX, const float inputVelY, const float inputVelZ)
    : id(inputId), mass(inputMass), x(inputX), y(inputY), z(inputZ), velX(inputVelX), velY(inputVelY), velZ(inputVelZ) {
}
