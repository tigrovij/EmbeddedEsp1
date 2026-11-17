#ifndef BUTTON_FSM_H
#define BUTTON_FSM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BUTTON_SHORT,
    BUTTON_DOUBLE,
    BUTTON_LONG,
    BUTTON_STUCK,
} button_event_t;

typedef enum {
    BUTTON_IDLE,
    BUTTON_FIRST_PRESS,
    BUTTON_WAITING_FOR_SECOND_PRESS,
    BUTTON_SECOND_PRESS,
    BUTTON_LONG_REPORTED,
    BUTTON_STUCK_REPORTED,
    BUTTON_STATE_COUNT,
} button_state_t;

typedef void (*button_event_cb_t)(button_event_t event, void *ctx);
typedef void (*button_state_cb_t)(button_state_t state, void *ctx);

typedef struct {
    button_state_cb_t callback;
    void *ctx;
} button_state_handler_t;

typedef struct {
    button_state_t state;
    bool raw_prev;
    bool stable_pressed;
    int64_t raw_change_us;
    int64_t press_start_us;
    int64_t first_release_us;
    button_state_handler_t handlers[BUTTON_STATE_COUNT];
} button_fsm_t;

// Supply monotonic timestamps in microseconds.
void button_fsm_init(button_fsm_t *fsm, int64_t now_us);

// Call after init. Each state has an optional handler and its own context.
// The handler runs once on entry during poll, before any associated event.
// Pass NULL as callback to remove the handler. Invalid states return false.
bool button_fsm_set_state_callback(button_fsm_t *fsm, button_state_t state,
                                   button_state_cb_t callback, void *ctx);

// Callbacks run synchronously and must not re-enter poll on this instance.
void button_fsm_poll(button_fsm_t *fsm, bool raw_pressed, int64_t now_us,
                     button_event_cb_t emit, void *ctx);

#endif
