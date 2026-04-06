#pragma once
#include <atomic>
#include "Particle.hpp"
#include <unordered_set>
#include <vector>
#include <nlohmann/json.hpp>

struct World {
    // Assume the minimum coordinates are always 0.0
    float max_x;
    float max_y;
    float max_z;
    float dt;
    float gravity_accel;

    // Counter for the particles ID assignment
    std::atomic<int> particleIdCounter = 1;

    std::vector<Particle> particles;

    World(float input_max_x = 1.0f, float input_max_y = 1.0f, float input_max_z = 1.0f, float input_dt = 1.0f,
          float input_gravity_accel = 9.81f);

    void step();

    void positionVelocityCalculation(float &pos, float &vel, float &accel, float &max) const;

    static float accelerationCalculation(float force, float mass);

    bool addParticle(const nlohmann::json &particleJson);

    bool addParticle(Particle &particle);

    [[nodiscard]] bool isValidParticle(const Particle &particle) const;

    void deleteParticleById(std::unordered_set<int> &idsToDelete);

    void fillSnapshot(World &copy) const;

    [[nodiscard]] bool isValid() const;
};

inline void to_json(nlohmann::json &j, const World &w) {
    j = nlohmann::json{
        {"max_x", w.max_x},
        {"max_y", w.max_y},
        {"max_z", w.max_z},
        {"gravity_accel", w.gravity_accel},
        {"particles", w.particles},
    };
}

inline void from_json(const nlohmann::json &j, World &w) {
    j.at("max_x").get_to(w.max_x);
    j.at("max_y").get_to(w.max_y);
    j.at("max_z").get_to(w.max_z);
    j.at("gravity_accel").get_to(w.gravity_accel);
    j.at("particles").get_to(w.particles);
}
