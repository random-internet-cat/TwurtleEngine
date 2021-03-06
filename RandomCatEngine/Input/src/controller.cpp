#include "randomcat/engine/input/controller.hpp"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>

namespace randomcat::engine {
    void controller::fetch_raw_events() noexcept {
        input::input_state_changes changes;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN: {
                    changes.keyboard().set_key_down(input_detail::wrap_key(event.key.keysym.sym));

                    break;
                }

                case SDL_KEYUP: {
                    changes.keyboard().set_key_up(input_detail::wrap_key(event.key.keysym.sym));

                    break;
                }

                case SDL_MOUSEMOTION: {
                    changes.mouse().delta_x() += event.motion.xrel;
                    changes.mouse().delta_y() += event.motion.yrel;

                    break;
                }

                case SDL_QUIT: {
                    m_quitReceived = true;

                    break;
                }
            }
        }

        m_currentInputState.update(changes);
        m_inputStateChanges = std::move(changes);
    }

    std::chrono::milliseconds controller::fetch_current_raw_time() const noexcept { return std::chrono::milliseconds{SDL_GetTicks()}; }
}    // namespace randomcat::engine
