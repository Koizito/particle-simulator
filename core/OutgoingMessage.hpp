#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct OutgoingMessage {
    std::string textData;
    std::vector<uint8_t> binaryData;

    OutgoingMessage() = default;

    explicit OutgoingMessage(std::string inputTextData);

    explicit OutgoingMessage(std::string inputTextData, std::vector<uint8_t> inputBinaryData);
};
