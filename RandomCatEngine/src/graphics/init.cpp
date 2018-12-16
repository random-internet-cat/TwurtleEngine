#include <SDL2/SDL.h>

#include <randomcat/engine/detail/log.hpp>
#include <randomcat/engine/graphics/init.hpp>

namespace randomcat::engine::graphics {
    namespace {
        bool g_initDone = false;
    }

    void init() {
        if (g_initDone) { log::warn("Graphics init called when already initialized!"); }

        g_initDone = true;

        log::info("Beginning graphics initialization...");

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        log::info("Graphics successfully initialized.");
    }
}    // namespace randomcat::engine::graphics
