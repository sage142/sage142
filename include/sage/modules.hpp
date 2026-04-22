#pragma once

#include "sage/publisher.hpp"
#include "sage/renderer.hpp"

#include <memory>

namespace sage {

std::unique_ptr<IRenderer> make_opengl_renderer();
std::unique_ptr<IPublisher> make_publisher();

} // namespace sage
