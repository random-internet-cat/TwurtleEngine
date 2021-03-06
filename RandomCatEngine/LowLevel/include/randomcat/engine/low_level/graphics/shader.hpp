#pragma once

#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include <randomcat/type_container/type_list.hpp>

#include "randomcat/engine/low_level/detail/tag_exception.hpp"
#include "randomcat/engine/low_level/graphics/gl_wrappers/active_locks.hpp"
#include "randomcat/engine/low_level/graphics/gl_wrappers/shader_program_raii.hpp"
#include "randomcat/engine/low_level/graphics/shader_input.hpp"
#include "randomcat/engine/low_level/graphics/shader_uniforms.hpp"

namespace randomcat::engine::graphics {
    namespace shader_detail {
        struct shader_init_error_tag {};
    }    // namespace shader_detail
    using shader_init_error = util_detail::tag_exception<shader_detail::shader_init_error_tag>;

    template<typename... Ts>
    using shader_capabilities = uniform_capabilities<Ts...>;

    using shader_no_capabilities = uniform_no_capabilities;

    template<typename Vertex, typename UniformCapabilities = shader_no_capabilities>
    class shader {
    public:
        static_assert(shader_detail::valid_capabilities<UniformCapabilities>, "UniformCapabilities must be valid");

        using vertex = Vertex;
        using uniform_manager = shader_uniform_writer<UniformCapabilities>;
        using const_uniform_manager = shader_uniform_writer<UniformCapabilities>;

        shader(shader const&) = delete;
        shader(shader&&) noexcept = default;

        shader& operator=(shader&&) noexcept = default;

        template<typename OtherCapabilities, typename = std::enable_if_t<type_container::type_list_is_sub_list_of_v<UniformCapabilities, OtherCapabilities>>>
        /* implicit */ shader(shader<Vertex, OtherCapabilities>&& _other) noexcept
        : m_programID(std::move(_other.program())), m_inputs(std::move(_other.inputs())) {}

        explicit shader(std::string_view _vertex, std::string_view _fragment, std::vector<shader_input> _inputs) noexcept(!"Throws on error");

        using active_lock = gl_detail::program_lock;

        [[nodiscard]] active_lock make_active_lock() const noexcept { return gl_detail::program_lock(program()); }

        [[nodiscard]] auto const& inputs() const noexcept { return m_inputs; }

        [[nodiscard]] auto uniforms() noexcept { return uniform_manager(program()); }
        [[nodiscard]] decltype(auto) uniforms() const noexcept { return const_uniforms(); }

        [[nodiscard]] auto const_uniforms() const noexcept { return const_uniform_manager(program()); }

        template<typename NewVertex>
        [[nodiscard]] shader<NewVertex, UniformCapabilities> reinterpret_vertex_and_inputs(std::vector<shader_input> _inputs) const noexcept;

        [[nodiscard]] shader<Vertex, UniformCapabilities> reinterpret_inputs(std::vector<shader_input> _inputs) const noexcept;

        template<typename NewVertex>
        [[nodiscard]] shader<NewVertex, UniformCapabilities> reinterpret_vertex() const noexcept;

        [[nodiscard]] shader clone() const noexcept;

        template<typename Func>
        [[nodiscard]] decltype(auto) uniforms_as(Func&& _func) noexcept(noexcept(uniforms().template as(std::forward<Func>(_func)))) {
            return uniforms().template as(std::forward<Func>(_func));
        }

        template<typename Func>
        [[nodiscard]] decltype(auto) uniforms_as(Func&& _func) const noexcept(noexcept(uniforms().template as(std::forward<Func>(_func)))) {
            return const_uniforms().template as(std::forward<Func>(_func));
        }

        template<typename UniformWrapper, typename = std::enable_if_t<type_container::type_list_contains_v<UniformCapabilities, UniformWrapper>>>
        [[nodiscard]] UniformWrapper uniforms_as() noexcept(noexcept(UniformWrapper(uniforms()))) {
            return uniforms().template as<UniformWrapper>();
        }

        template<typename UniformWrapper, typename = std::enable_if_t<type_container::type_list_contains_v<UniformCapabilities, UniformWrapper>>>
        [[nodiscard]] UniformWrapper uniforms_as() const noexcept(noexcept(const_uniforms().template as<UniformWrapper>())) {
            return const_uniforms().template as<UniformWrapper>();
        }

    protected:
        explicit shader(gl_detail::shared_program_id _program, std::vector<shader_input> _inputs) noexcept
        : m_programID(std::move(_program)), m_inputs(std::move(_inputs)) {}

        [[nodiscard]] auto const& program() const noexcept { return m_programID; }

    private:
        gl_detail::shared_program_id m_programID;
        std::vector<shader_input> m_inputs;

        template<typename, typename>
        friend class shader;

        template<typename, typename>
        friend class shader_view;
    };

    template<typename Vertex, typename UniformCapabilities = shader_no_capabilities>
    class shader_view {
    public:
        static_assert(shader_detail::valid_capabilities<UniformCapabilities>, "UniformCapabilities must be valid");

        template<typename OtherCapabilities, typename = std::enable_if_t<type_container::type_list_is_sub_list_of_v<UniformCapabilities, OtherCapabilities>>>
        /* implicit */ shader_view(shader<Vertex, OtherCapabilities> const& _other) noexcept(!"Copying vector")
        : shader_view(_other.program(), _other.inputs()) {}

        template<typename OtherCapabilities, typename = std::enable_if_t<type_container::type_list_is_sub_list_of_v<UniformCapabilities, OtherCapabilities>>>
        /* implicit */ shader_view(shader_view<Vertex, OtherCapabilities> const& _other) noexcept(!"Copying vector")
        : shader_view(_other.program(), _other.inputs()) {}

        using active_lock = gl_detail::program_lock;

        [[nodiscard]] active_lock make_active_lock() const noexcept { return gl_detail::program_lock(program()); }

        [[nodiscard]] auto const& inputs() const noexcept { return m_inputs; }

        using uniform_manager = shader_uniform_writer<UniformCapabilities>;
        using const_uniform_manager = uniform_manager;

        [[nodiscard]] const_uniform_manager uniforms() const noexcept { return const_uniform_manager(m_programID); }
        [[nodiscard]] const_uniform_manager const_uniforms() const noexcept { return uniforms(); }

        template<typename NewVertex>
        [[nodiscard]] shader<NewVertex, UniformCapabilities> reinterpret_vertex_and_inputs(std::vector<shader_input> _inputs) const noexcept;

        [[nodiscard]] shader<Vertex, UniformCapabilities> reinterpret_inputs(std::vector<shader_input> _inputs) const noexcept;

        template<typename NewVertex>
        [[nodiscard]] shader<NewVertex, UniformCapabilities> reinterpret_vertex() const noexcept;

        [[nodiscard]] shader<Vertex, UniformCapabilities> clone() const noexcept;

        template<typename Func>
        [[nodiscard]] decltype(auto) uniforms_as(Func&& _func) const noexcept(noexcept(uniforms().template as(std::forward<Func>(_func)))) {
            return const_uniforms().template as(std::forward<Func>(_func));
        }

        template<typename UniformManager, typename = std::enable_if_t<type_container::type_list_contains_v<UniformCapabilities, UniformManager>>>
        [[nodiscard]] UniformManager uniforms_as() const noexcept(noexcept(const_uniforms().template as<UniformManager>())) {
            return const_uniforms().template as<UniformManager>();
        }

    protected:
        explicit shader_view(gl_detail::shared_program_id _program, std::vector<shader_input> _inputs) noexcept(!"Copying vector")
        : m_programID(std::move(_program)), m_inputs(std::move(_inputs)) {}

        [[nodiscard]] auto const& program() const noexcept { return m_programID; }

    private:
        gl_detail::shared_program_id m_programID;
        std::vector<shader_input> m_inputs;
    };
}    // namespace randomcat::engine::graphics

#include "randomcat/engine/low_level/graphics/detail/shader_impl.inl"
