#pragma once
#include "Force.hpp"
#include <nlohmann/json.hpp>
#include <vector>

struct Particle {
    int id{};
    float mass{}; // grams
    float x{}; // meters
    float y{}; // meters
    float z{}; // meters
    float vel_x{}; // meters/second
    float vel_y{}; // meters/second
    float vel_z{}; // meters/second
    std::vector<Force> forces;

    Particle() = default;

    explicit Particle(int input_id, float input_mass=1.0f, float input_x=0.0f, float input_y=0.0f, float input_z=0.0f, float input_vel_x=0.0f, float input_vel_y=0.0f, float input_vel_z=0.0f);
};

inline void to_json(nlohmann::json& j, const Particle& p) {
    j = nlohmann::json{
        {"id", p.id},
        {"mass", p.mass},
        {"x", p.x},
        {"y", p.y},
        {"z", p.z},
        {"vel_x", p.vel_x},
        {"vel_y", p.vel_y},
        {"vel_z", p.vel_z},
        {"forces", p.forces}
    };
}

inline void from_json(const nlohmann::json& j, Particle& p) {
    j.at("id").get_to(p.id);
    j.at("mass").get_to(p.mass);
    j.at("x").get_to(p.x);
    j.at("y").get_to(p.y);
    j.at("z").get_to(p.z);
    j.at("vel_x").get_to(p.vel_x);
    j.at("vel_y").get_to(p.vel_y);
    j.at("vel_z").get_to(p.vel_z);
    j.at("forces").get_to(p.forces);
}
