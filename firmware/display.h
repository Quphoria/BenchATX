#pragma once

#include <stdint.h>
#include <stdbool.h>

// Use SH1106 instead of SSD1306
// #define SH1106

#define NUM_SCREENS 7
#define SETTINGS_SCREEN 6

void init_display(void);
void refresh_display(void);
void set_current_screen(uint8_t index);

void show_popup(uint8_t x, uint8_t y, uint8_t scale, uint16_t show_time_ms, const char *msg);
void show_popup_centered(uint8_t x, uint8_t y, uint8_t scale, uint16_t show_time_ms, uint8_t w, uint8_t h, const char *msg);

// Screen 0 FNS
void update_voltage(uint8_t index, int32_t voltage_mv);
void update_current(uint8_t index, int32_t current_100uA);
void update_overflow(uint8_t index, bool overflow);
void update_on_state(bool state);
void update_pwr_ok(bool ok);

// Settings screen FNS
void open_settings(void);
bool update_settings_menu(uint8_t btn1, uint8_t btn2); // Returns true to exit the screen
