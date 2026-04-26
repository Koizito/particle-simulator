#include "core/OutgoingMessage.hpp"

OutgoingMessage::OutgoingMessage(std::string inputTextData)
    : textData(std::move(inputTextData)) {
}

OutgoingMessage::OutgoingMessage(std::string inputTextData, std::vector<uint8_t> inputBinaryData)
    : textData(std::move(inputTextData)), binaryData(std::move(inputBinaryData)) {
}
