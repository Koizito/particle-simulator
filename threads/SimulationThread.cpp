#include "threads/SimulationThread.hpp"
#include <iostream>
#include "app/AppContext.hpp"
#include "core/Particle.hpp"

using clockAlias = std::chrono::steady_clock;
using timePoint = clockAlias::time_point;

SimulationThread::SimulationThread(AppContext& inputAppCtx)
    : BaseThread(inputAppCtx) {
}

void SimulationThread::runThread() {
    try {
        std::cout << "Starting the Particle Simulator \n\n";

        timePoint previous = clockAlias::now();

        while (!this->appCtx.shouldExit.load()) {
            waitForStartSignal();

            if (this->appCtx.shouldExit.load()) break;

            checkIfQueueIsFull();

            catchUpSimulation(previous);

            std::vector<Particle> particlesSnapshot = getParticlesSnapshot();

            nlohmann::json metadata = getSnapshotMetadata(particlesSnapshot);
            std::vector<uint8_t> bufferBytes = prepareSnapshotForSending(particlesSnapshot);

            queueForSending(metadata, bufferBytes);
        }
    } catch (const std::exception& e) {
        std::cerr << "Simulation thread error: " << e.what() << "\n";
        this->appCtx.signalExit();
    } catch (...) {
        std::cerr << "Simulation thread unknown error\n";
        this->appCtx.signalExit();
    }
}

void SimulationThread::waitForStartSignal() const {
    std::unique_lock<std::mutex> runLock(this->appCtx.simulationThreadMutex);
    this->appCtx.checkIfSimulationThreadShouldRun.wait(runLock, [this] {
        return this->appCtx.shouldSimulationThreadRun.load() || this->appCtx.shouldExit.load();
    });
}

void SimulationThread::checkIfQueueIsFull() const {
    std::unique_lock<std::mutex> runLock(this->appCtx.simulationThreadMutex);
    this->appCtx.checkIfSimulationThreadShouldRun.wait(runLock, [this] {
        return this->appCtx.normalSendQueue.size() > this->appCtx.MAX_QUEUE_SIZE;
    });
}

void SimulationThread::catchUpSimulation(timePoint& previous) const {
    int steps = 0;
    const timePoint now = clockAlias::now();

    while (now >= previous && steps < this->appCtx.MAX_STEPS_PER_FRAME) {
        std::lock_guard<std::mutex> stepLock(this->appCtx.worldMutex);
        this->appCtx.mainWorld.step();

        previous += std::chrono::duration_cast<clockAlias::duration>(
            std::chrono::duration<double>(this->appCtx.mainWorld.dt));
        steps++;
    }

    if (steps >= this->appCtx.MAX_STEPS_PER_FRAME) {
        previous = clockAlias::now();
    }
}

std::vector<Particle> SimulationThread::getParticlesSnapshot() const {
    std::lock_guard<std::mutex> lock(this->appCtx.worldMutex);
    return this->appCtx.mainWorld.particles;
}

nlohmann::json SimulationThread::getSnapshotMetadata(const std::vector<Particle>& particlesSnapshot) {
    size_t count = particlesSnapshot.size();

    nlohmann::json metadata;
    metadata["type"] = "particles";
    metadata["count"] = count;

    return metadata;
}

std::vector<uint8_t> SimulationThread::prepareSnapshotForSending(const std::vector<Particle>& particlesSnapshot) {
    const size_t count = particlesSnapshot.size();
    std::vector<uint8_t> bufferBytes;
    bufferBytes.reserve(count * sizeof(int) + count * 7 * sizeof(float));

    for (const auto& p: particlesSnapshot) {
        // ID
        const auto idPtr = reinterpret_cast<const uint8_t*>(&p.id);
        bufferBytes.insert(bufferBytes.end(), idPtr, idPtr + sizeof(int));

        // mass, position, velocity
        const float arr[] = {p.mass, p.x, p.y, p.z, p.velX, p.velY, p.velZ};
        const auto arrPtr = reinterpret_cast<const uint8_t*>(arr);
        bufferBytes.insert(bufferBytes.end(), arrPtr, arrPtr + sizeof(arr));
    }
    return bufferBytes;
}

void SimulationThread::queueForSending(const nlohmann::json& metadata, std::vector<uint8_t>& bufferBytes) const {
    std::lock_guard<std::mutex> lock(this->appCtx.sendThreadMutex);
    this->appCtx.normalSendQueue.emplace(metadata.dump());
    this->appCtx.normalSendQueue.emplace(std::move(bufferBytes));

    this->appCtx.checkIfSendThreadShouldRun.notify_one();
}
