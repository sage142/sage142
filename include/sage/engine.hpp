#pragma once

#include "sage/publisher.hpp"
#include "sage/renderer.hpp"

#include <memory>

namespace sage {

class Engine {
public:
    Engine(std::unique_ptr<IRenderer> renderer,
           std::unique_ptr<IPublisher> publisher);

    bool initialize();
    void run();
    void shutdown();

    bool publish(const PublishConfig& config);

private:
    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<IPublisher> publisher_;
    bool initialized_{false};
};

} // namespace sage
