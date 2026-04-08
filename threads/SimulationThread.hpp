#pragma once
#include "BaseThread.hpp"

class SimulationThread : public BaseThread {

public:
    void runThread() override;
    void waitForStartSignal();
    void checkIfQueueIsFull();
    void catchUpSimulation(std::chrono::steady_clock::time_point& previous);
    std::vector<Particle> getParticleSnapshot();
    nlohmann::json getSnapshotMetadata(const std::vector<Particle>& particles_snapshot);
    std::vector<uint8_t> prepareSnapshotForSending(const std::vector<Particle>& particles_snapshot, nlohmann::json& metadata);
    void queueForSending(const nlohmann::json& metadata, const std::vector<uint8_t>& buffer_bytes);
};