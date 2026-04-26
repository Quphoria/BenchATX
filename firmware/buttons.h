#pragma once

#include <stdbool.h>
#include "pico/time.h"

#define NUM_BTNS 2

#define SHORT_PRESS 1
#define LONG_PRESS 2

#define DEBOUNCE_DELAY_MS 50
#define LONG_PRESS_MS 800

static struct {
    const uint pins[NUM_BTNS];
    bool prev_state[NUM_BTNS];
    absolute_time_t debounce_delay[NUM_BTNS];
    bool long_press[NUM_BTNS];
} btn_status = {
    .pins = {6, 7},
    .prev_state = {0},
    .debounce_delay = {0},
    .long_press = {0},
};
