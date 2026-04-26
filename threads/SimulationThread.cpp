#include "threads/SimulationThread.hpp"
#include "app/AppContext.hpp"
#include "core/OutgoingMessage.hpp"
#include "core/Particle.hpp"

using clockAlias = std::chrono::steady_clock;
using timePoint = clockAlias::time_point;

SimulationThread::SimulationThread(std::shared_ptr<spdlog::logger> inputLogger, AppContext& inputAppCtx,
                                   int inputMaxStepsPerFrame)
    : BaseThread(std::move(inputLogger), "SimulationThread", inputAppCtx), MAX_STEPS_PER_FRAME(inputMaxStepsPerFrame) {
}

void SimulationThread::runThread() {
    try {
        logger->debug("[{}] Simulation loop started", threadType);

        timePoint previous = clockAlias::now();

        while (true) {
            this->appCtx.messagingQueue.waitForSpaceInNormalQueue(appCtx.shouldSendThreadRun, appCtx.shouldExit);

            if (this->appCtx.shouldExit.load()) break;

            catchUpSimulation(previous);

            std::vector<Particle> particlesSnapshot = getParticlesSnapshot();

            logger->debug("[{}] Getting snapshot metadata", threadType);
            nlohmann::json metadata = getSnapshotMetadata(particlesSnapshot);
            logger->debug("[{}] Preparing full snapshot for sending", threadType);
            std::vector<uint8_t> bufferBytes = prepareSnapshotForSending(particlesSnapshot);

            queueForSending(metadata, bufferBytes);
        }

        logger->debug("[{}] Simulation loop ended", threadType);
    } catch (const std::exception& e) {
        logger->error("[{}] Error: {}", threadType, e.what());
        this->appCtx.signalExit();
    } catch (...) {
        logger->error("[{}] Unknown error", threadType);
        this->appCtx.signalExit();
    }
}

void SimulationThread::catchUpSimulation(timePoint& previous) const {
    logger->debug("[{}] Catch up simulation", threadType);
    int steps = 0;
    const timePoint now = clockAlias::now();

    std::lock_guard<std::mutex> stepLock(this->appCtx.worldMutex);

    while (now >= previous && steps < MAX_STEPS_PER_FRAME) {
        this->appCtx.mainWorld.step();

        previous += std::chrono::duration_cast<clockAlias::duration>(
            std::chrono::duration<double>(this->appCtx.mainWorld.dt));
        steps++;
    }

    if (steps >= MAX_STEPS_PER_FRAME) {
        previous = clockAlias::now();
    }
}

std::vector<Particle> SimulationThread::getParticlesSnapshot() const {
    logger->debug("[{}] Getting snapshot of particles", threadType);
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

void SimulationThread::queueForSending(const nlohmann::json& metadata, const std::vector<uint8_t>& bufferBytes) const {
    logger->debug("[{}] Adding data to queue for sending", threadType);
    this->appCtx.messagingQueue.pushMessageToNormalQueue(OutgoingMessage(metadata.dump(), bufferBytes));
}
