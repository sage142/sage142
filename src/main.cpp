#include "sage/engine.hpp"
#include "sage/modules.hpp"

#include <iostream>

int main(int argc, char** argv) {
    sage::Engine engine(
        sage::make_opengl_renderer(),
        sage::make_publisher());

    if (argc > 1 && std::string(argv[1]) == "--publish") {
        sage::PublishConfig config;
        const bool ok = engine.publish(config);
        return ok ? 0 : 1;
    }

    if (!engine.initialize()) {
        return 1;
    }

    std::cout << "Engine running. Close the window to exit.\n";
    engine.run();
    engine.shutdown();
    return 0;
}
