#pragma once
#include "app/AppContext.hpp"
#include <iostream>

class BaseThread {
private:
    std::thread workerThread;

protected:
    AppContext &appCtx;

public:
    BaseThread(AppContext &inputAppCtx);
    void startThread();
    virtual void runThread() = 0;
    void stopThread();

    ~BaseThread();
};
