#include "core/OutgoingMessage.hpp"

OutgoingMessage::OutgoingMessage(std::string data) : textData(std::move(data)) {
}

OutgoingMessage::OutgoingMessage(std::vector<uint8_t> data) : binary(true), binaryData(std::move(data)) {
}
