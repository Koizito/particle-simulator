#pragma once
#include <string>
#include <unordered_set>
#include <spdlog/spdlog.h>

#include "nlohmann/json.hpp"
#include "core/World.hpp"

struct AppContext;

class MessageHandler {
    std::shared_ptr<spdlog::logger> logger;
    AppContext& appCtx;

public:
    MessageHandler(std::shared_ptr<spdlog::logger> inputLogger, AppContext& inputAppCtx);

    void handleMessage(const std::string& message);

    nlohmann::json parseMessage(const std::string& message);

    void addParticlesToWorld(const nlohmann::json& particlesJson);

    static bool particleJsonHasNonNumberFields(const nlohmann::json& particleJson);

    static bool particleJsonHasMissingFields(const nlohmann::json& particleJson);

    std::unordered_set<int> getParticleIdsFromJson(const nlohmann::json& particleIdsJson);

    World parseWorldFromJson(const nlohmann::json& jsonMessage);

    void updateWorld(World newWorld);

    World getWorldSnapshot();

    void pushWorldSnapshotToQueue(const World& snapshotWorld) const;
};
