#pragma once

#include <stdint.h>
#include <stdbool.h>

#define NUM_SCREENS 7

void init_display(void);
void refresh_display(void);
void set_current_screen(uint8_t index);

// Screen 0 FNS
void update_voltage(uint8_t index, int32_t voltage_mv);
void update_current(uint8_t index, int32_t current_100uA);
void update_overflow(uint8_t index, bool overflow);
void update_on_state(bool state);
void update_pwr_ok(bool ok);
