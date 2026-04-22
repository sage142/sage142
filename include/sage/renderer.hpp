#pragma once

#include <array>

namespace sage {

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual bool initialize() = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void shutdown() = 0;

    virtual bool should_close() const = 0;
    virtual void set_clear_color(const std::array<float, 4>& rgba) = 0;
};

} // namespace sage
