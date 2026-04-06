#pragma once
#include <nlohmann/json.hpp>

struct Force {
    float x; // newton
    float y; // newton
    float z; // newton

    Force(float input_x = 0.0f, float input_y = 0.0f, float input_z = 0.0f);
};

inline void to_json(nlohmann::json &j, const Force &f) {
    j = nlohmann::json{
        {"x", f.x},
        {"y", f.y},
        {"z", f.z},
    };
}

inline void from_json(const nlohmann::json &j, Force &f) {
    j.at("x").get_to(f.x);
    j.at("y").get_to(f.y);
    j.at("z").get_to(f.z);
}
