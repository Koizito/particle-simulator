#include "World.hpp"
#include <algorithm>
#include <cmath>

World::World(float input_max_x, float input_max_y, float input_max_z, float input_dt, float input_gravity_accel)
    : max_x(input_max_x), max_y(input_max_y), max_z(input_max_z), dt(input_dt), gravity_accel(input_gravity_accel) {}

void World::step() {
    
    for (Particle& particle : particles) {
        float particle_force_x = 0;
        float particle_force_y = 0;
        float particle_force_z = 0;

        for (Force& force : particle.forces){
            particle_force_x = particle_force_x + force.x;
            particle_force_y = particle_force_y + force.y;
            particle_force_z = particle_force_z + force.z;
        }

        float accel_x = accelerationCalculation(particle_force_x, particle.mass);
        float accel_y = accelerationCalculation(particle_force_y, particle.mass);
        float accel_z = accelerationCalculation(particle_force_z, particle.mass) + gravity_accel;

        positionVelocityCalculation(particle.x, particle.vel_x, accel_x, max_x);
        positionVelocityCalculation(particle.y, particle.vel_y, accel_y, max_y);
        positionVelocityCalculation(particle.z, particle.vel_z, accel_z, max_z);
    }
}

void World::positionVelocityCalculation(float& pos, float& vel, float& accel, float& max) const {
    float L = max;
    float period = 2*L;

    float newPos = pos + vel*dt + 0.5f*accel*dt*dt;

    float posRemainder = std::fmod(newPos, period);
    if (posRemainder < 0) posRemainder += period;

    if (posRemainder > L) {
        pos = period - posRemainder;
        vel = -(vel + accel*dt);
    } else {
        pos = posRemainder;
        vel = vel + accel*dt;
    }
}

float World::accelerationCalculation(float force, float mass) {
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
            particleJson.at("vel_x"),
            particleJson.at("vel_y"),
            particleJson.at("vel_z")
        );

        return addParticle(newParticle);
    } catch(nlohmann::json::out_of_range& e) {
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
    if (particle.x < 0 || particle.x > max_x) return false;
    if (particle.y < 0 || particle.y > max_y) return false;
    if (particle.z < 0 || particle.z > max_z) return false;

    // velocity validations
    if (!std::isfinite(particle.vel_x) || !std::isfinite(particle.vel_y) || !std::isfinite(particle.vel_z)) return false;

    float maxDisplacementX = 10 * max_x;
    float maxDisplacementY = 10 * max_y;
    float maxDisplacementZ = 10 * max_z;

    float displacementX = std::abs(particle.vel_x * dt);
    float displacementY = std::abs(particle.vel_y * dt);
    float displacementZ = std::abs(particle.vel_z * dt);

    if (displacementX > maxDisplacementX || displacementY > maxDisplacementY || displacementZ > maxDisplacementZ) return false;

    return true;
}

void World::deleteParticleById(std::unordered_set<int>& idsToDelete) {
    auto it = std::remove_if(particles.begin(), particles.end(),
        [&idsToDelete](const Particle& p) {
            return idsToDelete.find(p.id) != idsToDelete.end();
        });
        
    particles.erase(it, particles.end());
}

void World::fillSnapshot(World& copy) const {
    copy.max_x = max_x;
    copy.max_y = max_y;
    copy.max_z = max_z;
    copy.dt = dt;
    copy.gravity_accel = gravity_accel;
    copy.particles = particles;
    copy.particleIdCounter = particleIdCounter.load();
}