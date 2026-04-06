#include "SimulationThread.hpp"

SimulationThread::SimulationThread(AppContext &inputAppCtx)
    : appCtx(inputAppCtx) {
}

void SimulationThread::startThread() {
    try {
        workerThread = std::thread([this]() {
        std::cout << "Starting the Particle Simulator \n\n";

        using clock = std::chrono::steady_clock;
        auto next = clock::now();

        while (true) {
            {
                std::unique_lock<std::mutex> runLock(this->appCtx.simulationThreadMutex);
                this->appCtx.checkIfSimulationThreadShouldRun.wait(runLock, [this] {
                    return this->appCtx.shouldSimulationThreadRun || this->appCtx.shouldExit;
                });
            }

            if (this->appCtx.shouldExit) break;

            bool queueFull = false;
            {
                std::lock_guard<std::mutex> lock(this->appCtx.sendThreadMutex);
                queueFull = this->appCtx.normalSendQueue.size() > this->appCtx.MAX_QUEUE_SIZE;
            }

            if (queueFull) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            int steps = 0;
            auto now = clock::now();

            while (now >= next && steps < this->appCtx.MAX_STEPS_PER_FRAME) {
                {
                    std::lock_guard<std::mutex> stepLock(this->appCtx.worldMutex);
                    this->appCtx.mainWorld.step();
                }

                next += std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(this->appCtx.mainWorld.dt));
                steps++;
            }

            if (steps >= this->appCtx.MAX_STEPS_PER_FRAME) {
                next = clock::now();
            }

            std::vector<Particle> particles_snapshot;
            {
                std::lock_guard<std::mutex> lock(this->appCtx.worldMutex);
                particles_snapshot = this->appCtx.mainWorld.particles;
            }

            size_t count = particles_snapshot.size();

            nlohmann::json metadata;
            metadata["type"] = "particles";
            metadata["count"] = count;

            std::vector<uint8_t> buffer_bytes;
            buffer_bytes.reserve(count * sizeof(int) + count * 7 * sizeof(float));

            for (const auto &p: particles_snapshot) {
                // ID
                auto id_ptr = reinterpret_cast<const uint8_t *>(&p.id);
                buffer_bytes.insert(buffer_bytes.end(), id_ptr, id_ptr + sizeof(int));

                // mass, position, velocity
                const float arr[] = {p.mass, p.x, p.y, p.z, p.vel_x, p.vel_y, p.vel_z};
                auto arr_ptr = reinterpret_cast<const uint8_t *>(arr);
                buffer_bytes.insert(buffer_bytes.end(), arr_ptr, arr_ptr + sizeof(arr));
            }

            {
                std::lock_guard<std::mutex> lock(this->appCtx.sendThreadMutex);
                this->appCtx.normalSendQueue.emplace(metadata.dump());
                this->appCtx.normalSendQueue.emplace(std::move(buffer_bytes));
            }
            this->appCtx.checkIfSendThreadShouldRun.notify_one();
        }
    });
    } catch (const std::exception& e) {
        std::cerr << "Simulation thread error: " << e.what() << "\n";
        this->appCtx.signalExit();
    } catch (...) {
        std::cerr << "Simulation thread unknown error\n";
        this->appCtx.signalExit();
    }

}

void SimulationThread::stopThread() {
    if (workerThread.joinable()) {
        workerThread.join();
    }
}
