#pragma once

#include "sage/publisher.hpp"
#include "sage/renderer.hpp"
#include "sage/script_system.hpp"

#include <memory>

namespace sage {

std::unique_ptr<IRenderer> make_opengl_renderer();
std::unique_ptr<IScriptSystem> make_lua_script_system();
std::unique_ptr<IPublisher> make_publisher();

} // namespace sage
