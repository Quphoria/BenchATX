#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "ssd1306.h"

#include "display.h"

#include "Pix32_Font.h"
#include "On_Icon.h"


static ssd1306_t disp;

static uint8_t current_screen = 0;
static bool dirty = true;
static union {
    struct {
        int32_t voltage_mv[5];
        int32_t current_100uA[5];
        bool on_state;
        bool pwr_ok;
    } screen0;
} st = {0};

void update_voltage(uint8_t index, int32_t voltage_mv) {
    if (index >= 5) return;
    if (current_screen == 0) {
        dirty |= st.screen0.voltage_mv[index] != voltage_mv;
    }
    st.screen0.voltage_mv[index] = voltage_mv;
}

void update_current(uint8_t index, int32_t current_100uA) {
    if (index >= 5) return;
    if (current_screen == 0) {
        dirty |= st.screen0.current_100uA[index] != current_100uA;
    }
    st.screen0.current_100uA[index] = current_100uA;
}

void update_on_state(bool state) {
    if (current_screen == 0) {
        dirty |= st.screen0.on_state != state;
    }
    st.screen0.on_state = state;
}

void update_pwr_ok(bool ok) {
    if (current_screen == 0) {
        dirty |= st.screen0.pwr_ok != ok;
    }
    st.screen0.pwr_ok = ok;
}

static void draw_string(ssd1306_t *p, uint32_t x, uint32_t y, uint32_t scale, const uint8_t *font, const char *s) {
    uint32_t sx = x;
    uint32_t dx = (font[1]+font[2]) * scale;
    uint32_t dy = (font[0]+font[2]) * scale;

    while (*s != 0) {
        if (*s == '\n') {
            x = sx;
            y += dy;
        } else {
            ssd1306_draw_char_with_font(p, x, y, scale, font, *s);
            x += dx;
        }
        s++;
    }
}

static inline uint8_t print_voltage(char *s, uint8_t n, int32_t voltage_mv) {
    if (abs(voltage_mv) < 1000) { // < 1V
        // 123mV
        return snprintf(s, n, "%3dmV", voltage_mv);
    }
    voltage_mv /= 10; // Convert to 10mV
    uint8_t mv = abs(voltage_mv) % 100;
    voltage_mv /= 100; // Convert to volts
    if (abs(voltage_mv) >= 10) {
        // 12.1V
        return snprintf(s, n, "%2d.%01dV", voltage_mv, mv / 10);
    } else {
        // 5.01V
        return snprintf(s, n, "%1d.%02dV", voltage_mv, mv);
    }
}

static inline uint8_t print_current(char *s, uint8_t n, int32_t current_100uA) {
    if (abs(current_100uA) < 1000) { // < 100mA
        // 12.3mA
        return snprintf(s, n, "%2d.%1dmA", current_100uA / 10, abs(current_100uA) % 10);
    }
    current_100uA /= 10; // Convert to mA
    uint16_t ma = abs(current_100uA) % 1000;
    current_100uA /= 1000; // Convert to amps
    if (abs(current_100uA) >= 10) {
        // 12.34A
        return snprintf(s, n, "%2d.%02dA", current_100uA, ma / 10);
    } else {
        // 1.234A
        return snprintf(s, n, "%1d.%03dA", current_100uA, ma);
    }
}

static void draw_screen0() {
    char s[15*5];
    char *sp = s;
    uint8_t n = sizeof(s);
    uint8_t w = 0;

    for (uint8_t i = 0; i < 5; i++) {
        w = print_voltage(sp, n, st.screen0.voltage_mv[i]);
        sp += w;
        n -= w;
        
        if (n+1 < sizeof(s)) {
            *(sp++) = ' ';
            n -= 1;
        }

        w = print_current(sp, n, st.screen0.current_100uA[i]);
        sp += w;
        n -= w;

        if (n+1 < sizeof(s) && i != 4) {
            *(sp++) = '\n';
            n -= 1;
        }
    }

    if (n+1 >= sizeof(s)) {
        printf("Error: String overflow rendering screen 0");
    }

    draw_string(&disp, 1, 2, 1, Pix32_Font, s);

    if (st.screen0.pwr_ok) {
        draw_string(&disp, 128-(Pix32_Font[1]*6)-2, 64-Pix32_Font[0]-2, 1, Pix32_Font, "PWR OK");
    }

    if (st.screen0.on_state) {
        ssd1306_draw_char_with_font(&disp, 128-(On_Icon[1]*2)-2, 32-On_Icon[0], 2, On_Icon, 'A');
    }
}

static int anim_x = 0;
static int anim_y = 0;

static void draw_anim(void) {
    ssd1306_draw_square(&disp, anim_x, 0, 2, 64);
    anim_x = (anim_x + 1) % 128;
    ssd1306_draw_square(&disp, 0, anim_y, 128, 2);
    anim_y = (anim_y + 1) % 64;
}

void init_display(void) {
    disp.external_vcc=false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c0);
    /* Possible changes to init cmds
    
    SET_CONTRAST,
    0xCF, // From https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.cpp#L598
    ...
    SET_VCOM_DESEL,
    0x40, // From https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.cpp#L618
    */

    dirty = true;
    ssd1306_clear(&disp);
    // ssd1306_draw_square(&disp, 0, 0, 128, 64);
    ssd1306_show(&disp);
}

void set_current_screen(uint8_t index) {
    printf("Current screen: %0d\n", index);
    current_screen = index;
    dirty = true;
}

void refresh_display(void) {
    if (!dirty) return;
    dirty = false;

    ssd1306_clear(&disp);
    
    switch (current_screen) {
        case 0:
            draw_screen0();
            break;
        case 10:
            draw_anim();
            dirty = true; // Redraw every frame
            break;
    }

    ssd1306_show(&disp);
}
