#include "OutgoingMessage.hpp"

OutgoingMessage::OutgoingMessage(std::string data) : binary(false), textData(std::move(data)) {
}

OutgoingMessage::OutgoingMessage(std::vector<uint8_t> data) : binary(true), binaryData(std::move(data)) {
}
