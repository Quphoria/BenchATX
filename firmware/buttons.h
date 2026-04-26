#pragma once

#include <stdbool.h>
#include "pico/time.h"

#define NUM_BTNS 2

static struct {
    const uint pins[NUM_BTNS];
    bool prev_state[NUM_BTNS];
    absolute_time_t debounce_delay[NUM_BTNS];
} btn_status = {
    .pins = {6, 7},
    .prev_state = {0},
    .debounce_delay = {0},
};
