#include "OutgoingMessage.hpp"
#include <string>

OutgoingMessage::OutgoingMessage(bool input_binary, std::string input_data) : binary(input_binary), data(std::move(input_data)) {}