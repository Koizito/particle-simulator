#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

struct DecodedParticle {
    int id;
    float mass, x, y, z, velX, velY, velZ;
};

inline std::vector<DecodedParticle> decodeSnapshot(const std::vector<uint8_t>& binary) {
    const size_t particleSize = sizeof(int) + 7 * sizeof(float);
    const size_t count = binary.size() / particleSize;
    std::vector<DecodedParticle> particles(count);

    const uint8_t* ptr = binary.data();
    for (size_t i = 0; i < count; ++i) {
        DecodedParticle& p = particles[i];
        std::memcpy(&p.id,   ptr,                     sizeof(int));   ptr += sizeof(int);
        std::memcpy(&p.mass, ptr,                     sizeof(float)); ptr += sizeof(float);
        std::memcpy(&p.x,    ptr,                     sizeof(float)); ptr += sizeof(float);
        std::memcpy(&p.y,    ptr,                     sizeof(float)); ptr += sizeof(float);
        std::memcpy(&p.z,    ptr,                     sizeof(float)); ptr += sizeof(float);
        std::memcpy(&p.velX, ptr,                     sizeof(float)); ptr += sizeof(float);
        std::memcpy(&p.velY, ptr,                     sizeof(float)); ptr += sizeof(float);
        std::memcpy(&p.velZ, ptr,                     sizeof(float)); ptr += sizeof(float);
    }
    return particles;
}