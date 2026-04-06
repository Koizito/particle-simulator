#pragma once
#include "app/AppContext.hpp"
#include <iostream>

class SendThread {
    AppContext &appCtx;
    std::thread workerThread;

public:
    SendThread(AppContext &inputAppCtx);
    void startThread();
    void stopThread();

    ~SendThread() {
        stopThread();
    }
};
