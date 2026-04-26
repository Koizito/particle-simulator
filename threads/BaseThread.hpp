#pragma once
#include <memory>
#include <string>
#include <thread>
#include <spdlog/spdlog.h>

struct AppContext;

class BaseThread {
    std::thread workerThread;

protected:
    std::shared_ptr<spdlog::logger> logger;
    std::string threadType;
    AppContext& appCtx;

public:
    BaseThread(std::shared_ptr<spdlog::logger> inputLogger, std::string inputThreadType, AppContext& inputAppCtx);

    void startThread();

    virtual void runThread() = 0;

    void stopThread();

    virtual ~BaseThread();
};
