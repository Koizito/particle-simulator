#pragma once
#include "threads/BaseThread.hpp"
#include "core/Particle.hpp"
#include <vector>

class AppContext;

class SimulationThread : public BaseThread {
public:
    explicit SimulationThread(AppContext& inputAppCtx);

    void runThread() override;

    void waitForStartSignal() const;

    void checkIfQueueIsFull() const;

    void catchUpSimulation(std::chrono::steady_clock::time_point& previous) const;

    [[nodiscard]] std::vector<Particle> getParticlesSnapshot() const;

    static nlohmann::json getSnapshotMetadata(const std::vector<Particle>& particlesSnapshot);

    static std::vector<uint8_t> prepareSnapshotForSending(const std::vector<Particle>& particlesSnapshot);

    void queueForSending(const nlohmann::json& metadata, std::vector<uint8_t>& bufferBytes) const;
};
