#include "core/World.hpp"

#include "Constants.hpp"

World::World(const float inputMaxX, const float inputMaxY, const float inputMaxZ, const float inputDt,
             const float inputGravityAccel, const float inputAirDensity, const float inputDragCoefficient,
             const float inputAirViscosity)
    : maxX(inputMaxX), maxY(inputMaxY), maxZ(inputMaxZ), dt(inputDt), gravityAccel(inputGravityAccel),
      airDensity(inputAirDensity), dragCoefficient(inputDragCoefficient), airViscosity(inputAirViscosity) {
}

void World::step() {
    for (Particle& particle: particles) {
        float particleForceX = 0;
        float particleForceY = 0;
        float particleForceZ = 0;

        for (const Force& force: particle.forces) {
            particleForceX = particleForceX + force.x;
            particleForceY = particleForceY + force.y;
            particleForceZ = particleForceZ + force.z;
        }

        calculateAirDrag(particle, particleForceX, particleForceY, particleForceZ);

        float accelX = accelerationCalculation(particleForceX, particle.mass);
        float accelY = accelerationCalculation(particleForceY, particle.mass);
        float accelZ = accelerationCalculation(particleForceZ, particle.mass) + gravityAccel;

        positionVelocityCalculation(particle.x, particle.velX, accelX, maxX);
        positionVelocityCalculation(particle.y, particle.velY, accelY, maxY);
        positionVelocityCalculation(particle.z, particle.velZ, accelZ, maxZ);
    }
}

void World::positionVelocityCalculation(float& position, float& velocity, const float& acceleration,
                                        const float& maximum) const {
    const float L = maximum;
    const float period = 2 * L;

    const float newPos = position + velocity * dt + 0.5f * acceleration * dt * dt;

    float positionRemainder = std::fmod(newPos, period);
    if (positionRemainder < 0) positionRemainder += period;

    if (positionRemainder > L) {
        position = period - positionRemainder;
        velocity = -(velocity + acceleration * dt);
    } else {
        position = positionRemainder;
        velocity = velocity + acceleration * dt;
    }
}

float World::accelerationCalculation(const float& force, const float& mass) {
    return force / (mass / 1000); // 1000 because mass is in grams.
}

bool World::addParticle(Particle& particle) {
    if (isValidParticle(particle)) {
        particles.push_back(std::move(particle));
        return true;
    }
    return false;
}

bool World::isValidParticle(const Particle& particle) const {
    // mass validations
    if (!std::isfinite(particle.mass) || particle.mass < 1e-6f || particle.mass > 1e6f) return false;

    // coordinates validations
    if (!std::isfinite(particle.x) || !std::isfinite(particle.y) || !std::isfinite(particle.z)) return false;
    if (particle.x < 0 || particle.x > maxX) return false;
    if (particle.y < 0 || particle.y > maxY) return false;
    if (particle.z < 0 || particle.z > maxZ) return false;

    // velocity validations
    if (!std::isfinite(particle.velX) || !std::isfinite(particle.velY) || !std::isfinite(particle.velZ))
        return
                false;

    const float maxDisplacementX = 10 * maxX;
    const float maxDisplacementY = 10 * maxY;
    const float maxDisplacementZ = 10 * maxZ;

    const float displacementX = std::abs(particle.velX * dt);
    const float displacementY = std::abs(particle.velY * dt);
    const float displacementZ = std::abs(particle.velZ * dt);

    if (displacementX > maxDisplacementX || displacementY > maxDisplacementY || displacementZ > maxDisplacementZ)
        return
                false;

    return true;
}

void World::deleteParticlesById(std::unordered_set<int>& idsToDelete) {
    const auto it = std::remove_if(particles.begin(), particles.end(),
                                   [&idsToDelete](const Particle& p) {
                                       return idsToDelete.find(p.id) != idsToDelete.end();
                                   });

    particles.erase(it, particles.end());
}

void World::fillSnapshot(World& copy) const {
    copy.maxX = maxX;
    copy.maxY = maxY;
    copy.maxZ = maxZ;
    copy.dt = dt;
    copy.gravityAccel = gravityAccel;
    copy.particles = particles;
}

bool World::isValid() const {
    // dimension validations
    if (!std::isfinite(maxX) || !std::isfinite(maxY) || !std::isfinite(maxZ)) return false;
    if (maxX < 1.0f || maxY < 1.0f || maxZ < 1.0f) return false;
    if (maxX > 1e6f || maxY > 1e6f || maxZ > 1e6f) return false;
    // timestep validation
    if (dt <= 0.0f || dt > 0.1f) return false;
    // gravity acceleration validation
    if (!std::isfinite(gravityAccel) || gravityAccel > 1e6f) return false;

    return true;
}

bool World::canUpdateBounds(const float newMaxX, const float newMaxY, const float newMaxZ) const {
    for (const auto& particle: particles) {
        if (particle.x > newMaxX || particle.y > newMaxY || particle.z > newMaxZ) return false;
    }
    return true;
}

void World::calculateAirDrag(const Particle& particle, float& particleForceX, float& particleForceY,
                             float& particleForceZ) const {
    const float speed = std::sqrt(particle.velX * particle.velX +
                                  particle.velY * particle.velY +
                                  particle.velZ * particle.velZ);
    if (speed < 1e-6f) return;

    const float area = constants::PI * particle.radius * particle.radius;
    const float quadraticForce = 0.5f * airDensity * dragCoefficient * area * speed * speed; // Drag Equation

    const float linearForce = constants::SIX_PI * airViscosity * particle.radius * speed; // Stoke's Law

    const float totalDrag = quadraticForce + linearForce;

    const float invSpeed = 1.0f / speed;
    const float fx = -totalDrag * particle.velX * invSpeed;
    const float fy = -totalDrag * particle.velY * invSpeed;
    const float fz = -totalDrag * particle.velZ * invSpeed;

    particleForceX += fx;
    particleForceY += fy;
    particleForceZ += fz;
}
