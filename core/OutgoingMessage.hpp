#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct OutgoingMessage {
    bool binary;
    std::string textData;
    std::vector<uint8_t> binaryData;

    OutgoingMessage() = default;

    OutgoingMessage(std::string data);

    OutgoingMessage(std::vector<uint8_t> data);
};