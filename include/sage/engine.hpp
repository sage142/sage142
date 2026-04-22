#pragma once

#include "sage/publisher.hpp"
#include "sage/renderer.hpp"
#include "sage/script_system.hpp"

#include <memory>
#include <string>

namespace sage {

class Engine {
public:
    Engine(std::unique_ptr<IRenderer> renderer,
           std::unique_ptr<IScriptSystem> script_system,
           std::unique_ptr<IPublisher> publisher);

    bool initialize(const std::string& bootstrap_script_path);
    void run();
    void shutdown();

    bool publish(const PublishConfig& config);

private:
    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<IScriptSystem> script_system_;
    std::unique_ptr<IPublisher> publisher_;
    bool initialized_{false};
};

} // namespace sage
