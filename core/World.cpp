#include "World.hpp"

World::World(const float inputMaxX, const float inputMaxY, const float inputMaxZ, const float inputDt,
             const float inputGravityAccel)
    : maxX(inputMaxX), maxY(inputMaxY), maxZ(inputMaxZ), dt(inputDt), gravityAccel(inputGravityAccel) {
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

        float accelX = accelerationCalculation(particleForceX, particle.mass);
        float accelY = accelerationCalculation(particleForceY, particle.mass);
        float accelZ = accelerationCalculation(particleForceZ, particle.mass) + gravityAccel;

        positionVelocityCalculation(particle.x, particle.velX, accelX, maxX);
        positionVelocityCalculation(particle.y, particle.velY, accelY, maxY);
        positionVelocityCalculation(particle.z, particle.velZ, accelZ, maxZ);
    }
}

void World::positionVelocityCalculation(float& position, float& velocity, const float& acceleration, const float& maximum) const {
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

bool World::addParticle(const nlohmann::json& particleJson) {
    try {
        Particle newParticle(
            particleIdCounter.fetch_add(1),
            particleJson.at("mass"),
            particleJson.at("x"),
            particleJson.at("y"),
            particleJson.at("z"),
            particleJson.at("velX"),
            particleJson.at("velY"),
            particleJson.at("velZ")
        );

        return addParticle(newParticle);
    } catch (nlohmann::json::out_of_range& e) {
        return false;
    }
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

void World::deleteParticleById(std::unordered_set<int>& idsToDelete) {
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
    copy.particleIdCounter = particleIdCounter.load();
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
