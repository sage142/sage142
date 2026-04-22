#pragma once

#include <memory>
#include <string>

namespace sage {
class IRenderer;

class IScriptSystem {
public:
    virtual ~IScriptSystem() = default;

    virtual bool initialize(IRenderer* renderer) = 0;
    virtual bool run_file(const std::string& path) = 0;
    virtual void shutdown() = 0;
};

} // namespace sage
