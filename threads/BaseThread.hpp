#pragma once
#include <thread>

struct AppContext;

class BaseThread {
    std::thread workerThread;

protected:
    AppContext& appCtx;

public:
    explicit BaseThread(AppContext& inputAppCtx);

    void startThread();

    virtual void runThread() = 0;

    void stopThread();

    virtual ~BaseThread();
};
