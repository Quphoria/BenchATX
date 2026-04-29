#pragma once

#include <stdint.h>
#include <stdbool.h>

#define NUM_SETTINGS 3
#define SETTING_CONTRAST        1
#define SETTING_STARTUP_STATE   2
#define SETTING_EN_LOGGING      3

#define CONTRAST_STEP 0x11
#define LOGGING_PLAIN   1
#define LOGGING_CSV     2

typedef struct __packed__ {
    const uint8_t ver;      // Settings version, increment when changing settings struct
    uint8_t disp_contrast;  // The display contrast
    uint8_t startup_state;  // The initial PSU on state (0: off, 1: on)
    uint8_t uart_logging;   // Enable logging to UART/USB (0: off, 1: on, 2: csv)
} settings_t;

extern settings_t settings;

void load_settings(void);
void save_settings(void);
void load_default_settings(void);
