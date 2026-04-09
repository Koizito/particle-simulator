#pragma once
#include "BaseThread.hpp"
#include <iostream>

class SimulationThread : public BaseThread {
public:
    explicit SimulationThread(AppContext& inputAppCtx);

    void runThread() override;

    void waitForStartSignal() const;

    void checkIfQueueIsFull() const;

    void catchUpSimulation(std::chrono::steady_clock::time_point& previous) const;

    std::vector<Particle> getParticleSnapshot() const;

    static nlohmann::json getSnapshotMetadata(const std::vector<Particle>& particlesSnapshot);

    static std::vector<uint8_t> prepareSnapshotForSending(const std::vector<Particle>& particlesSnapshot,
                                                          nlohmann::json& metadata);

    void queueForSending(const nlohmann::json& metadata, std::vector<uint8_t>& bufferBytes) const;
};
