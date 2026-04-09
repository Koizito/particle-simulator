#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct OutgoingMessage {
    bool binary = false;
    std::string textData;
    std::vector<uint8_t> binaryData;

    OutgoingMessage() = default;

    explicit OutgoingMessage(std::string data);

    explicit OutgoingMessage(std::vector<uint8_t> data);
};
