#include "sage/engine.hpp"

#include <iostream>

namespace sage {

Engine::Engine(std::unique_ptr<IRenderer> renderer,
               std::unique_ptr<IPublisher> publisher)
    : renderer_(std::move(renderer)),
      publisher_(std::move(publisher)) {}

bool Engine::initialize() {
    if (!renderer_) {
        std::cerr << "Engine not constructed with required renderer module.\n";
        return false;
    }

    if (!renderer_->initialize()) {
        std::cerr << "Renderer initialization failed.\n";
        return false;
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
