#pragma once

#include <GL/glew.h>

#include "randomcat/engine/low_level/graphics/gl_wrappers/opengl_raii_id.hpp"

namespace randomcat::engine::graphics::gl_detail {
    [[nodiscard]] inline auto make_texture() noexcept {
        opengl_raw_id id;
        glGenTextures(1, &id);
        return id;
    }

    inline void destroy_texture(opengl_raw_id _id) noexcept { glDeleteTextures(1, &_id); }

    using unique_texture_id = unique_opengl_raii_id<make_texture, destroy_texture>;
    using shared_texture_id = shared_opengl_raii_id<make_texture, destroy_texture>;
    using raw_texture_id = unique_texture_id::raw_id;
}    // namespace randomcat::engine::graphics::gl_detail
