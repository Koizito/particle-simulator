#pragma once
#include "Force.hpp"
#include <nlohmann/json.hpp>
#include <vector>

struct Particle {
    int id;
    float mass; // grams
    float x; // meters
    float y; // meters
    float z; // meters
    float velX; // meters/second
    float velY; // meters/second
    float velZ; // meters/second
    std::vector<Force> forces;

    Particle() = default;

    explicit Particle(int inputId, float inputMass = 1.0f, float inputX = 0.0f, float inputY = 0.0f,
                      float inputZ = 0.0f, float inputVelX = 0.0f, float inputVelY = 0.0f,
                      float inputVelZ = 0.0f);
};

inline void to_json(nlohmann::json& json, const Particle& particle) {
    json = nlohmann::json{
        {"id", particle.id},
        {"mass", particle.mass},
        {"x", particle.x},
        {"y", particle.y},
        {"z", particle.z},
        {"velX", particle.velX},
        {"velY", particle.velY},
        {"velZ", particle.velZ},
        {"forces", particle.forces}
    };
}

inline void from_json(const nlohmann::json& json, Particle& particle) {
    json.at("id").get_to(particle.id);
    json.at("mass").get_to(particle.mass);
    json.at("x").get_to(particle.x);
    json.at("y").get_to(particle.y);
    json.at("z").get_to(particle.z);
    json.at("velX").get_to(particle.velX);
    json.at("velY").get_to(particle.velY);
    json.at("velZ").get_to(particle.velZ);
    json.at("forces").get_to(particle.forces);
}
