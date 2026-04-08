#include "SimulationThread.hpp"

using clock_alias = std::chrono::steady_clock;
using time_point = clock_alias::time_point;

SimulationThread::SimulationThread(AppContext& inputAppCtx)
    : BaseThread(inputAppCtx) {
}

void SimulationThread::runThread() {
    try {
        std::cout << "Starting the Particle Simulator \n\n";

        time_point previous = clock_alias::now();

        while (!this->appCtx.shouldExit.load()) {
            waitForStartSignal();

            if (this->appCtx.shouldExit.load()) break;

            checkIfQueueIsFull();

            catchUpSimulation(previous);

            std::vector<Particle> particles_snapshot = getParticleSnapshot();

            nlohmann::json metadata = getSnapshotMetadata(particles_snapshot);
            std::vector<uint8_t> buffer_bytes = prepareSnapshotForSending(particles_snapshot, metadata);

            queueForSending(metadata, buffer_bytes);
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

void SimulationThread::catchUpSimulation(time_point& previous) const {
    int steps = 0;
    const time_point now = clock_alias::now();

    while (now >= previous && steps < this->appCtx.MAX_STEPS_PER_FRAME) {
        std::lock_guard<std::mutex> stepLock(this->appCtx.worldMutex);
        this->appCtx.mainWorld.step();

        previous += std::chrono::duration_cast<clock_alias::duration>(
            std::chrono::duration<double>(this->appCtx.mainWorld.dt));
        steps++;
    }

    if (steps >= this->appCtx.MAX_STEPS_PER_FRAME) {
        previous = clock_alias::now();
    }
}

std::vector<Particle> SimulationThread::getParticleSnapshot() const {
    std::lock_guard<std::mutex> lock(this->appCtx.worldMutex);
    return this->appCtx.mainWorld.particles;
}

nlohmann::json SimulationThread::getSnapshotMetadata(const std::vector<Particle>& particles_snapshot) {
    size_t count = particles_snapshot.size();

    nlohmann::json metadata;
    metadata["type"] = "particles";
    metadata["count"] = count;

    return metadata;
}

std::vector<uint8_t> SimulationThread::prepareSnapshotForSending(const std::vector<Particle>& particles_snapshot,
                                                                 nlohmann::json& metadata) {
    const size_t count = particles_snapshot.size();
    std::vector<uint8_t> buffer_bytes;
    buffer_bytes.reserve(count * sizeof(int) + count * 7 * sizeof(float));

    for (const auto& p: particles_snapshot) {
        // ID
        const auto id_ptr = reinterpret_cast<const uint8_t*>(&p.id);
        buffer_bytes.insert(buffer_bytes.end(), id_ptr, id_ptr + sizeof(int));

        // mass, position, velocity
        const float arr[] = {p.mass, p.x, p.y, p.z, p.vel_x, p.vel_y, p.vel_z};
        const auto arr_ptr = reinterpret_cast<const uint8_t*>(arr);
        buffer_bytes.insert(buffer_bytes.end(), arr_ptr, arr_ptr + sizeof(arr));
    }
    return buffer_bytes;
}

void SimulationThread::queueForSending(const nlohmann::json& metadata, std::vector<uint8_t>& buffer_bytes) const {
    std::lock_guard<std::mutex> lock(this->appCtx.sendThreadMutex);
    this->appCtx.normalSendQueue.emplace(metadata.dump());
    this->appCtx.normalSendQueue.emplace(std::move(buffer_bytes));

    this->appCtx.checkIfSendThreadShouldRun.notify_one();
}
