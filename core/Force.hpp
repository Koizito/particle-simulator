#pragma once
#include <nlohmann/json.hpp>

struct Force {
    float x; // newton
    float y; // newton
    float z; // newton

    explicit Force(float inputX = 0.0f, float inputY = 0.0f, float inputZ = 0.0f);
};

inline void to_json(nlohmann::json& json, const Force& force) {
    json = nlohmann::json{
        {"x", force.x},
        {"y", force.y},
        {"z", force.z},
    };
}

inline void from_json(const nlohmann::json& json, Force& force) {
    json.at("x").get_to(force.x);
    json.at("y").get_to(force.y);
    json.at("z").get_to(force.z);
}
