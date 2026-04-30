#pragma once
#include <thread>

class ThreadJoiner {
    std::thread& thread;

public:
    explicit ThreadJoiner(std::thread& t) : thread(t) {}
    ~ThreadJoiner() {
        if (thread.joinable()) {
            thread.join();
        }
    }

    ThreadJoiner(const ThreadJoiner&) = delete;
    ThreadJoiner& operator=(const ThreadJoiner&) = delete;
};