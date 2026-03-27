#include "OutgoingMessage.hpp"
#include <string>

    // text
OutgoingMessage::OutgoingMessage(std::string data) : binary(false), textData(std::move(data)) {}

    // binary
OutgoingMessage::OutgoingMessage(std::vector<uint8_t> data) : binary(true), binaryData(std::move(data)) {}