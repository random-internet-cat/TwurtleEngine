#pragma once

#include <GL/glew.h>

#include "randomcat/engine/low_level/graphics/gl_wrappers/opengl_raii_id.hpp"

namespace randomcat::engine::graphics::gl_detail {
    [[nodiscard]] inline auto make_shader(GLenum _type) noexcept { return opengl_raw_id{glCreateShader(_type)}; }

    inline void destroy_shader(opengl_raw_id _id) noexcept { glDeleteShader(_id); }

    [[nodiscard]] inline auto make_program() noexcept { return opengl_raw_id{glCreateProgram()}; }

    inline void destroy_program(opengl_raw_id _id) noexcept { glDeleteProgram(_id); }

    using unique_shader_id = unique_opengl_raii_id<make_shader, destroy_shader>;
    using shared_shader_id = shared_opengl_raii_id<make_shader, destroy_shader>;
    using raw_shader_id = unique_shader_id::raw_id;

    using unique_program_id = unique_opengl_raii_id<make_program, destroy_program>;
    using shared_program_id = shared_opengl_raii_id<make_program, destroy_program>;
    using raw_program_id = unique_program_id::raw_id;
}    // namespace randomcat::engine::graphics::gl_detail
