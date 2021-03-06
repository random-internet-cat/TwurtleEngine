#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
//#include <vector>

#include <GL/glew.h>
#include <glm/ext.hpp>

#include <randomcat/engine/input/controller.hpp>
#include <randomcat/engine/input/input_state.hpp>
#include <randomcat/engine/input/keycodes.hpp>
#include <randomcat/engine/low_level/graphics/gl_wrappers/texture_raii.hpp>
#include <randomcat/engine/low_level/graphics/render_context.hpp>
#include <randomcat/engine/low_level/graphics/shader.hpp>
#include <randomcat/engine/low_level/init.hpp>
#include <randomcat/engine/low_level/window.hpp>
#include <randomcat/engine/render_objects/graphics/default_vertex.hpp>
#include <randomcat/engine/render_objects/graphics/object.hpp>
#include <randomcat/engine/textures/graphics/color_texture.hpp>
#include <randomcat/engine/textures/graphics/texture_binder.hpp>
#include <randomcat/engine/textures/graphics/texture_fs.hpp>
#include <randomcat/engine/textures/graphics/texture_manager.hpp>
#include <randomcat/engine/utilities/graphics/camera.hpp>
#include <randomcat/units/default_units.hpp>
#include <randomcat/units/units.hpp>

#include "custom_shader.hpp"

using namespace randomcat;
using namespace randomcat::engine;
using namespace randomcat::engine::graphics;
using namespace randomcat::engine::input;

static_assert(std::numeric_limits<float>::is_iec559);
static_assert(std::numeric_limits<double>::is_iec559);
static_assert(std::numeric_limits<long double>::is_iec559);

namespace {
    glm::vec3 process_movement(input::keyboard_input_state const& _inputState, glm::vec3 const& _camDir, std::chrono::milliseconds _delta) {
        constexpr auto zero_y = [](glm::vec3 vec) { return glm::vec3(vec.x, 0, vec.z); };
        constexpr auto movementSpeed = 0.01f;

        glm::vec3 movement{0.0f};
        glm::vec3 verticalMovement{0.0f};

        if (_inputState.key_is_down(input::keycode::kc_w)) movement += normalize(zero_y(_camDir));

        if (_inputState.key_is_down(input::keycode::kc_a))
            movement += -normalize(zero_y(cross(normalize(_camDir), glm::vec3{0.0f, 1.0f, 0.0f})));

        if (_inputState.key_is_down(input::keycode::kc_s)) movement += -normalize(zero_y(_camDir));

        if (_inputState.key_is_down(input::keycode::kc_d))
            movement += normalize(zero_y(cross(normalize(_camDir), glm::vec3{0.0f, 1.0f, 0.0f})));

        if (_inputState.key_is_down(input::keycode::kc_space)) verticalMovement += glm::vec3{0, 1, 0};

        if (_inputState.key_is_down(input::keycode::kc_lshift)) verticalMovement += glm::vec3{0, -1, 0};

        return ((movement != glm::vec3{0.0f} ? glm::normalize(movement) : glm::vec3{0.0f}) + verticalMovement) * movementSpeed
               * static_cast<float>(_delta.count());
    }

    std::chrono::seconds constexpr BENCHMARK_TIME = std::chrono::seconds(1000);
    bool constexpr DO_BENCHMARK = true;
}    // namespace

namespace units = ::randomcat::units;

int main() {
    using vertex = basic_game::lighting_vertex;
    using renderer = vertex_renderer<vertex>;
    using render_cube = render_object_cube<>;
    using game_object = render_object_rect_prism<vertex>;

    try {
        using namespace std::chrono_literals;

        randomcat::engine::init();

        window window{"Twurtle Engine", 800, 800};
        auto renderContext = render_context(window, render_context::flags::debug);
        auto renderContextLock = renderContext.make_active_lock();
        auto engine = controller();
        auto theShader = basic_game::custom_shader();

        window.set_cursor_shown(false);

        textures::texture_manager textureManager;

        auto const& wallImage = load_texture_file(textureManager, "texture/wall.jpg");
        auto const& crossImage = load_texture_file(textureManager, "texture/cross.png");
        auto const& textImage = load_texture_file(textureManager, "texture/text.jpg");
        auto const& translucencyImage = load_texture_file(textureManager, "texture/translucency.png");
        auto const& colorImage = textureManager.add_texture("color", textures::color_texture(16, 16, color_rgb{1, 1, 1}));

        auto const& [textureArray_, wallTexture_, textTexture_, crossTexture_, translucencyTexture_, colorTexture_] =
            textures::make_texture_array_from_layers(wallImage, textImage, crossImage, translucencyImage, colorImage);

        auto halfTexture = [](textures::texture_rectangle const& _rect) {
            auto const dimensions = _rect.dimensions();
            auto const& xDim = dimensions.x;
            auto const& yDim = dimensions.y;
            return textures::texture_rectangle{_rect.layer(), textures::texture_rectangle::from_corner_and_dimensions, _rect.top_left(), xDim * 0.5f, yDim};
        };

        [[maybe_unused]] auto const& textureArray = textureArray_;
        [[maybe_unused]] auto const& wallTexture = halfTexture(wallTexture_);
        [[maybe_unused]] auto const& textTexture = halfTexture(textTexture_);
        [[maybe_unused]] auto const& crossTexture = crossTexture_;
        [[maybe_unused]] auto const& translucencyTexture = translucencyTexture_;
        [[maybe_unused]] auto const& colorTexture = colorTexture_;

        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

        auto camPos = glm::vec3{4.5f, 15.0f, 4.5f};

        auto distanceToCam = [&](auto const& cube) { return glm::distance(camPos, cube.center()); };

        auto yaw = units::degrees(0);
        auto pitch = units::degrees(-90);
        float constexpr sensitivity = 0.1f;

        std::chrono::milliseconds currentTime = engine.timer().current_time();
        auto lastPlace = currentTime;

        static_assert(std::is_same_v<decltype(decompose_render_object_to<void>(12, nullptr)), std::nullptr_t>);

        std::vector<game_object> objects;

        auto vertexVecRenderer = renderer(theShader);

        auto const aspectRatio = window.aspect_ratio();

        auto cam = theShader.uniforms().as<camera>();

        auto lastSecondTime = currentTime;

        unsigned long totalFrames = 0;

        constexpr auto roundVec3 = [](glm::vec3 vec) { return glm::vec3{round(vec.x), round(vec.y), round(vec.z)}; };

        std::vector<vertex> vertices;
        vertices.reserve(100 * 36);

        [[maybe_unused]] auto const textCube = [&](glm::vec3 pos) { return render_cube{pos, 1, textTexture}; };
        [[maybe_unused]] auto const wallCube = [&](glm::vec3 pos) { return render_cube{pos, 1, wallTexture}; };
        [[maybe_unused]] auto const crossCube = [&](glm::vec3 pos) { return render_cube{pos, 1, crossTexture}; };
        [[maybe_unused]] auto const translucencyCube = [&](glm::vec3 pos) { return render_cube{pos, 1, translucencyTexture}; };
        [[maybe_unused]] auto const colorCube = [&](glm::vec3 pos) { return render_cube(pos, 1, colorTexture); };

        auto reflectivity = glm::vec3(0.5, 0.5, 0.5);
        auto const blockMaterial = vertex::material_t{.ambient = reflectivity, .specular = reflectivity, .diffuse = reflectivity, .shininess = 32};

        auto const toGameVertex = [&](auto const& oldVertex) {
            return basic_game::lighting_vertex{oldVertex.location, oldVertex.texture, oldVertex.normal, blockMaterial};
        };

        auto const addUserBlock = [&](render_cube obj) { objects.push_back(obj.use_vertex<basic_game::lighting_vertex>(toGameVertex)); };

        auto const matchesBlock = [&](glm::vec3 pos) {
            return [rounded = roundVec3(pos)](auto const& obj) { return obj.center() == rounded; };
        };

        auto const hasUserBlockAtPos = [&](glm::vec3 pos) { return std::any_of(begin(objects), end(objects), matchesBlock(pos)); };

        auto const removeUserBlock = [&](glm::vec3 pos) {
            auto lastValid = std::remove_if(begin(objects), end(objects), matchesBlock(pos));
            auto removedAny = lastValid != end(objects);

            objects.erase(lastValid, end(objects));

            return removedAny;
        };

        auto const genCameraState = [&](position pos, direction dir) noexcept {
            return camera_state{
                .aspectRatio = aspectRatio,
                .fov = units::degrees(45),
                .minDistance = 0.1f,
                .maxDistance = 100.f,
                .dir = dir,
                .pos = pos,
            };
        };

        if constexpr (DO_BENCHMARK) {
            for (int x = 0; x < 10; x++) {
                for (int z = 0; z < 10; z++) { addUserBlock(textCube({x, 0, z})); }
            }
        }

        //set_global_background_color(color_rgb{.r = 0.6, .g = 0.6, .b = 0.9});

        auto lightHandler = theShader.uniforms_as<light_handler>();

        lightHandler.add_light({.position = glm::vec3{8, 5, 8},
                                .colors = {.ambient = {1, 0, 0}, .specular = {1, 0, 0}, .diffuse = {1, 0, 0}},
                                .attenuation = {.constant = 1.f, .linear = 0.09f, .quadratic = 0.032f}});

        lightHandler.add_light({.position = glm::vec3{5, 5, 5},
                                .colors = {.ambient = {0, 1, 0}, .specular = {0, 1, 0}, .diffuse = {0, 1, 0}},
                                .attenuation = {.constant = 1.f, .linear = 0.09f, .quadratic = 0.032f}});

        lightHandler.add_light({.position = glm::vec3{2, 5, 2},
                                .colors = {.ambient = {0, 0, 1}, .specular = {0, 0, 1}, .diffuse = {0, 0, 1}},
                                .attenuation = {.constant = 1.f, .linear = 0.09f, .quadratic = 0.032f}});

        lightHandler.update();

        while (true) {
            engine.tick();
            auto const& inputState = engine.inputs();
            auto const& inputChanges = engine.input_changes();

            currentTime = engine.timer().current_time();

            if constexpr (DO_BENCHMARK) {
                if (currentTime - engine.timer().start_time() > BENCHMARK_TIME) return 0;
            }

            ++totalFrames;

            if (currentTime > lastSecondTime + 1s) {
                lastSecondTime = currentTime;
                log::info << "FPS: " << engine.timer().fps();
                log::info << "Objects: " << objects.size();
            }

            auto camDir = as_glm({.yaw = yaw, .pitch = pitch});

            {
                auto oneBlockInDir = glm::normalize(camDir);

                if (currentTime > lastPlace + 100ms) {
                    if (inputState.keyboard().key_is_down(input::keycode::kc_e)) {
                        lastPlace = currentTime;

                        for (auto blockNum = 1; blockNum <= 4; ++blockNum) {
                            auto const testPos = camPos + oneBlockInDir * static_cast<float>(blockNum);
                            if (hasUserBlockAtPos(testPos)) { addUserBlock(textCube(roundVec3(testPos - oneBlockInDir))); }
                        }
                    }

                    if (inputState.keyboard().key_is_down(input::keycode::kc_q)) {
                        lastPlace = currentTime;

                        for (auto blockNum = 0; blockNum <= 4; ++blockNum) {
                            auto const testPos = camPos + oneBlockInDir * static_cast<float>(blockNum);

                            auto removedAny = removeUserBlock(testPos);
                            if (removedAny) { break; }
                        }
                    }
                }
            }

            renderContext.render([&] {
                vertices.clear();
                std::sort(begin(objects), end(objects), [&](auto const& first, auto const& second) {
                    return distanceToCam(second) < distanceToCam(first);
                });
                decompose_render_object_to<vertex>(begin(objects), end(objects), std::back_inserter(vertices));
                decompose_render_object_to<vertex>(render_object_regular_polygon<default_vertex>(currentTime.count() / 1000, {0, 5, 0}, 4, wallTexture)
                                                       .use_vertex<vertex>(toGameVertex),
                                                   std::back_inserter(vertices));
                vertexVecRenderer(vertices);
            });

            yaw += units::degrees(inputChanges.mouse().delta_x() * sensitivity);
            pitch += units::degrees(-inputChanges.mouse().delta_y() * sensitivity);

            pitch = units::degrees(std::clamp(units::degrees(pitch).count(), -89.9, 89.9));

            if (inputState.keyboard().key_is_down(input::keycode::kc_escape)) return 0;

            if (inputState.keyboard().key_is_down(input::keycode::kc_r)) objects.clear();

            camPos += process_movement(inputState.keyboard(), camDir, engine.timer().delta_time());

            cam.update(genCameraState(position(camPos), direction({.yaw = yaw, .pitch = pitch})));

            if (engine.quit_received()) return 0;
        }
    } catch (std::exception& e) {
        log::error << "Exception reached main! Message: " << e.what();
        return 1;
    } catch (...) {
        log::error("Exception reached main!");
        return 1;
    }
}
