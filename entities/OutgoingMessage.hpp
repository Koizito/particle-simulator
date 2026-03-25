#include <string>

struct OutgoingMessage  {
    bool binary;
    std::string data;
    
    OutgoingMessage(bool input_binary=false, std::string input_data="");
};

