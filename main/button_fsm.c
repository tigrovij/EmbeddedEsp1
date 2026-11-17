#include "button_fsm.h"

#include <stddef.h>

#define DEBOUNCE_US       30000LL
#define DOUBLE_CLICK_US  350000LL
#define LONG_PRESS_US   2000000LL
#define STUCK_US        7000000LL

static void publish(button_event_cb_t emit, button_event_t event, void *ctx)
{
    if (emit != NULL) {
        emit(event, ctx);
    }
}

static void enter_state(button_fsm_t *fsm, button_state_t state)
{
    fsm->state = state;
    button_state_handler_t handler = fsm->handlers[state];
    if (handler.callback != NULL) {
        handler.callback(state, handler.ctx);
    }
}

void button_fsm_init(button_fsm_t *fsm, int64_t now_us)
{
    *fsm = (button_fsm_t){
        .state = BUTTON_IDLE,
        .raw_change_us = now_us,
    };
}

bool button_fsm_set_state_callback(button_fsm_t *fsm, button_state_t state,
                                   button_state_cb_t callback, void *ctx)
{
    if ((unsigned int)state >= BUTTON_STATE_COUNT) {
        return false;
    }
    fsm->handlers[state] = (button_state_handler_t){callback, ctx};
    return true;
}

void button_fsm_poll(button_fsm_t *fsm, bool raw_pressed, int64_t now_us,
                     button_event_cb_t emit, void *ctx)
{
    if (raw_pressed != fsm->raw_prev) {
        fsm->raw_prev = raw_pressed;
        fsm->raw_change_us = now_us;
    }

    bool pressed = false;
    bool released = false;
    if (raw_pressed != fsm->stable_pressed &&
        now_us - fsm->raw_change_us >= DEBOUNCE_US) {
        fsm->stable_pressed = raw_pressed;
        pressed = raw_pressed;
        released = !raw_pressed;
    }

    const int64_t held_us = now_us - fsm->press_start_us;
    switch (fsm->state) {
    case BUTTON_IDLE:
        if (pressed) {
            fsm->press_start_us = now_us;
            enter_state(fsm, BUTTON_FIRST_PRESS);
        }
        break;

    case BUTTON_FIRST_PRESS:
    case BUTTON_SECOND_PRESS:
        if (released) {
            if (fsm->state == BUTTON_SECOND_PRESS) {
                enter_state(fsm, BUTTON_IDLE);
                publish(emit, BUTTON_DOUBLE, ctx);
            } else {
                fsm->first_release_us = now_us;
                enter_state(fsm, BUTTON_WAITING_FOR_SECOND_PRESS);
            }
            break;
        }
        if (held_us >= LONG_PRESS_US) {
            enter_state(fsm, BUTTON_LONG_REPORTED);
            publish(emit, BUTTON_LONG, ctx);
        }
        // A delayed poll can cross both hold thresholds.
        if (held_us >= STUCK_US) {
            enter_state(fsm, BUTTON_STUCK_REPORTED);
            publish(emit, BUTTON_STUCK, ctx);
        }
        break;

    case BUTTON_WAITING_FOR_SECOND_PRESS:
        if (now_us - fsm->first_release_us >= DOUBLE_CLICK_US) {
            enter_state(fsm, BUTTON_IDLE);
            publish(emit, BUTTON_SHORT, ctx);
            if (pressed) {
                fsm->press_start_us = now_us;
                enter_state(fsm, BUTTON_FIRST_PRESS);
            }
        } else if (pressed) {
            fsm->press_start_us = now_us;
            enter_state(fsm, BUTTON_SECOND_PRESS);
        }
        break;

    case BUTTON_LONG_REPORTED:
        if (released) {
            enter_state(fsm, BUTTON_IDLE);
        } else if (held_us >= STUCK_US) {
            enter_state(fsm, BUTTON_STUCK_REPORTED);
            publish(emit, BUTTON_STUCK, ctx);
        }
        break;

    case BUTTON_STUCK_REPORTED:
        if (released) {
            enter_state(fsm, BUTTON_IDLE);
        }
        break;

    case BUTTON_STATE_COUNT:
        break;
    }
}
