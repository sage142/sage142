#include "sage/modules.hpp"

#include <lua.hpp>

#include <array>
#include <iostream>

namespace sage {

namespace {
IRenderer* g_renderer = nullptr;

int lua_log(lua_State* L) {
    const char* message = luaL_checkstring(L, 1);
    std::cout << "[Lua] " << message << "\n";
    return 0;
}

int lua_set_clear_color(lua_State* L) {
    const float r = static_cast<float>(luaL_checknumber(L, 1));
    const float g = static_cast<float>(luaL_checknumber(L, 2));
    const float b = static_cast<float>(luaL_checknumber(L, 3));
    const float a = static_cast<float>(luaL_optnumber(L, 4, 1.0));

    if (g_renderer) {
        g_renderer->set_clear_color({r, g, b, a});
    }

    return 0;
}
} // namespace

class LuaScriptSystem final : public IScriptSystem {
public:
    bool initialize(IRenderer* renderer) override {
        g_renderer = renderer;
        L_ = luaL_newstate();
        if (!L_) {
            std::cerr << "Failed to create Lua state.\n";
            return false;
        }

        luaL_openlibs(L_);
        lua_register(L_, "log", lua_log);
        lua_register(L_, "set_clear_color", lua_set_clear_color);

        return true;
    }

    bool run_file(const std::string& path) override {
        if (!L_) {
            return false;
        }

        if (luaL_dofile(L_, path.c_str()) != LUA_OK) {
            std::cerr << "Lua error: " << lua_tostring(L_, -1) << "\n";
            lua_pop(L_, 1);
            return false;
        }

        return true;
    }

    void shutdown() override {
        if (L_) {
            lua_close(L_);
            L_ = nullptr;
        }
        g_renderer = nullptr;
    }

private:
    lua_State* L_{nullptr};
};

std::unique_ptr<IScriptSystem> make_lua_script_system() {
    return std::make_unique<LuaScriptSystem>();
}

} // namespace sage
