#pragma once
#include "BaseThread.hpp"

class Sendthread : public BaseThread {

public:
    void runThread() override;
};