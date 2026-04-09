#pragma once
#include "core/Particle.hpp"
#include <unordered_set>
#include <vector>
#include <nlohmann/json.hpp>

struct AppContext;

struct World {
    // Assume the minimum coordinates are always 0.0
    float maxX;
    float maxY;
    float maxZ;
    float dt;
    float gravityAccel;

    std::vector<Particle> particles;

    explicit World(float inputMaxX = 1.0f, float inputMaxY = 1.0f, float inputMaxZ = 1.0f, float inputDt = 0.1f,
                   float inputGravityAccel = 9.81f);

    void step();

    void positionVelocityCalculation(float& position, float& velocity, const float& acceleration,
                                     const float& maximum) const;

    static float accelerationCalculation(const float& force, const float& mass);

    bool addParticle(Particle& particle);

    [[nodiscard]] bool isValidParticle(const Particle& particle) const;

    void deleteParticlesById(std::unordered_set<int>& idsToDelete);

    void fillSnapshot(World& copy) const;

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] bool canUpdateBounds(float newMaxX, float newMaxY, float newMaxZ) const;
};

inline void to_json(nlohmann::json& json, const World& world) {
    json = nlohmann::json{
        {"maxX", world.maxX},
        {"maxY", world.maxY},
        {"maxZ", world.maxZ},
        {"dt", world.dt},
        {"gravityAccel", world.gravityAccel},
        {"particles", world.particles},
    };
}

inline void from_json(const nlohmann::json& json, World& world) {
    json.at("maxX").get_to(world.maxX);
    json.at("maxY").get_to(world.maxY);
    json.at("maxZ").get_to(world.maxZ);
    json.at("dt").get_to(world.dt);
    json.at("gravityAccel").get_to(world.gravityAccel);
    json.at("particles").get_to(world.particles);
}
