#pragma once
#include "threads/BaseThread.hpp"
#include "core/Particle.hpp"
#include <vector>

struct AppContext;

class SimulationThread : public BaseThread {
    // Max steps each frame of the simulation can attempt to make before synchronization is forced
    int MAX_STEPS_PER_FRAME;

public:
    SimulationThread(std::shared_ptr<spdlog::logger> inputLogger, AppContext& inputAppCtx,
                     int inputMaxStepsPerFrame = 10);

    void runThread() override;

    void waitForRunSignal() const;

    void catchUpSimulation(std::chrono::steady_clock::time_point& previous) const;

    [[nodiscard]] std::vector<Particle> getParticlesSnapshot() const;

    static nlohmann::json getSnapshotMetadata(const std::vector<Particle>& particlesSnapshot);

    static std::vector<uint8_t> prepareSnapshotForSending(const std::vector<Particle>& particlesSnapshot);

    void queueForSending(const nlohmann::json& metadata, const std::vector<uint8_t>& bufferBytes) const;
};
