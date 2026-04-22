#include "sage/engine.hpp"

#include <iostream>

namespace sage {

Engine::Engine(std::unique_ptr<IRenderer> renderer,
               std::unique_ptr<IScriptSystem> script_system,
               std::unique_ptr<IPublisher> publisher)
    : renderer_(std::move(renderer)),
      script_system_(std::move(script_system)),
      publisher_(std::move(publisher)) {}

bool Engine::initialize(const std::string& bootstrap_script_path) {
    if (!renderer_ || !script_system_) {
        std::cerr << "Engine not constructed with required modules.\n";
        return false;
    }

    if (!renderer_->initialize()) {
        std::cerr << "Renderer initialization failed.\n";
        return false;
    }

    if (!script_system_->initialize(renderer_.get())) {
        std::cerr << "Script system initialization failed.\n";
        renderer_->shutdown();
        return false;
    }

    if (!script_system_->run_file(bootstrap_script_path)) {
        std::cerr << "Bootstrap script failed: " << bootstrap_script_path << "\n";
    }

    initialized_ = true;
    return true;
}

void Engine::run() {
    if (!initialized_) {
        std::cerr << "Cannot run engine before initialization.\n";
        return;
    }

    while (!renderer_->should_close()) {
        renderer_->begin_frame();
        renderer_->end_frame();
    }
}

void Engine::shutdown() {
    if (!initialized_) {
        return;
    }

    script_system_->shutdown();
    renderer_->shutdown();
    initialized_ = false;
}

bool Engine::publish(const PublishConfig& config) {
    if (!publisher_) {
        std::cerr << "No publisher module configured.\n";
        return false;
    }
    return publisher_->publish(config);
}

} // namespace sage
