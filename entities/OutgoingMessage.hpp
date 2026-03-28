#include <cstdint>
#include <string>
#include <vector>

struct OutgoingMessage {
    bool binary;
    std::string textData;
    std::vector<uint8_t> binaryData;

    // text
    OutgoingMessage(std::string data);

    // binary
    OutgoingMessage(std::vector<uint8_t> data);
};