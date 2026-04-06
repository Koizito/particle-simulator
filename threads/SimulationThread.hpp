#pragma once
#include "app/AppContext.hpp"
#include <iostream>

class SimulationThread {
    AppContext &appCtx;
    std::thread workerThread;

public:
    SimulationThread(AppContext &inputAppCtx);
    void startThread();
    void stopThread();

    ~SimulationThread() {
        stopThread();
    }
};