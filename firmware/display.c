#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ssd1306.h"

#include "settings.h"
#include "display.h"
#include "buttons.h"

#include "Pix32_Font.h"
#include "Pix32_Inv_Font.h"
#include "On_Icon.h"
#include "Chan_Icons.h"

#define POPUP_SHOW 2
#define POPUP_SHOWN 1

static ssd1306_t disp;

static uint8_t current_screen = 0;
static bool dirty = true;
static struct {
    struct {
        int32_t voltage_mv[5];
        int32_t current_100uA[5];
        bool overflow[5];
        bool on_state;
        bool pwr_ok;
    } screen0;
    struct {
        settings_t tmp;
        uint8_t index;
        bool sel;
        bool is_open;
        bool modified;
        bool forced_contrast;
    } settings;
    struct {
        char msg[32];
        uint8_t x;
        uint8_t y;
        uint8_t scale;
        absolute_time_t hide_time;
        uint8_t state;
    } popup;
} st = {0};

static void draw_string(ssd1306_t *p, uint32_t x, uint32_t y, uint32_t scale, const uint8_t *font, const char *s);
static void draw_string_with_inverts(ssd1306_t *p, uint32_t x, uint32_t y, uint32_t scale, const char *s);
static inline uint8_t print_voltage(char *s, uint8_t n, int32_t voltage_mv, bool large);
static inline uint8_t print_current(char *s, uint8_t n, int32_t current_100uA, bool large);

static void draw_screen0();
static void draw_screen1_5();
static void draw_settings_menu();
// static void draw_anim();

void update_voltage(uint8_t index, int32_t voltage_mv) {
    if (index >= 5) return;
    if (current_screen <= 5) {
        dirty |= st.screen0.voltage_mv[index] != voltage_mv;
    }
    st.screen0.voltage_mv[index] = voltage_mv;
}

void update_current(uint8_t index, int32_t current_100uA) {
    if (index >= 5) return;
    if (current_screen <= 5) {
        dirty |= st.screen0.current_100uA[index] != current_100uA;
    }
    st.screen0.current_100uA[index] = current_100uA;
}

void update_overflow(uint8_t index, bool overflow) {
    if (index >= 5) return;
    if (current_screen <= 5) {
        dirty |= st.screen0.overflow[index] != overflow;
    }
    st.screen0.overflow[index] = overflow;
}

void update_on_state(bool state) {
    if (current_screen <= 5) {
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

void init_display(void) {
    disp.external_vcc=false;
#ifdef SH1106
    disp.is_sh1106=true;
#endif
    ssd1306_init(&disp, 128, 64, 0x3C, i2c0);
    ssd1306_contrast(&disp, settings.disp_contrast);
    printf("Contrast: 0x%02x\n", settings.disp_contrast);

    dirty = true;
    ssd1306_clear(&disp);
    // ssd1306_draw_square(&disp, 0, 0, 128, 64);
    ssd1306_show(&disp);
}

void set_current_screen(uint8_t index) {
    printf("Current screen: %0d\n", index);
    current_screen = index;
    if (current_screen == SETTINGS_SCREEN) {
        st.settings.is_open = false;
    }
    dirty = true;
}

void show_popup(uint8_t x, uint8_t y, uint8_t scale, uint16_t show_time_ms, const char *msg) {
    st.popup.x = x;
    st.popup.y = y;
    st.popup.scale = scale;
    strncpy(st.popup.msg, msg, sizeof(st.popup.msg));
    st.popup.hide_time = make_timeout_time_ms(show_time_ms);
    st.popup.state = POPUP_SHOW;
    dirty = true;
}

void show_popup_centered(uint8_t x, uint8_t y, uint8_t scale, uint16_t show_time_ms, uint8_t w, uint8_t h, const char *msg) {
    int16_t x2 = x*2; // Center using half pixels, then floor to nearest pixel
    int16_t y2 = y*2;

    x2 -= scale * w * Pix32_Font[1];
    y2 -= scale * h * Pix32_Font[0];

    show_popup(x2/2, y2/2, scale, show_time_ms, msg);
}

void refresh_display(void) {
    if (!dirty) return;
    dirty = false;

    if (st.popup.state) {
        if (absolute_time_diff_us(get_absolute_time(), st.popup.hide_time) < 0) {
            st.popup.state = 0;
            ssd1306_contrast(&disp, settings.disp_contrast);
        } else {
            if (st.popup.state == POPUP_SHOW) {
                st.popup.state = POPUP_SHOWN;
                ssd1306_clear(&disp);
                draw_string_with_inverts(&disp, st.popup.x, st.popup.y, st.popup.scale, st.popup.msg);
                ssd1306_show(&disp);
                ssd1306_contrast(&disp, 0xff); // Force contrast for popup
            }
            dirty = true;
            return;
        }
    }

    ssd1306_clear(&disp);

    switch (current_screen) {
        case 0:
            draw_screen0();
            break;
        case 1 ... 5:
            draw_screen1_5();
            break;
        case SETTINGS_SCREEN:
            draw_settings_menu();
            break;
        // case 7:
        //     draw_anim();
        //     dirty = true; // Redraw every frame
        //     break;
    }

    ssd1306_show(&disp);

    if (current_screen == SETTINGS_SCREEN) {
        if (!st.settings.forced_contrast) {
            st.settings.forced_contrast = true;
            ssd1306_contrast(&disp, 0xff); // Force contrast for settings menu
        }
    } else if (st.settings.forced_contrast) {
        st.settings.forced_contrast = false;
        ssd1306_contrast(&disp, settings.disp_contrast);
    }
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

static void draw_string_with_inverts(ssd1306_t *p, uint32_t x, uint32_t y, uint32_t scale, const char *s) {
    uint32_t sx = x;
    uint32_t dx = (Pix32_Font[1]+Pix32_Font[2]) * scale;
    uint32_t dy = (Pix32_Font[0]+Pix32_Font[2]) * scale;
    bool inverted = false;

    while (*s != 0) {
        if (*s == '\n') {
            x = sx;
            y += dy;
        } else if (*s =='\a') { // Bell character
            inverted = !inverted;
        } else {
            ssd1306_draw_char_with_font(p, x, y, scale, inverted ? Pix32_Inv_Font : Pix32_Font, *s);
            x += dx;
        }
        s++;
    }
}

static inline uint8_t print_voltage(char *s, uint8_t n, int32_t voltage_mv, bool large) {
    if (large) {
        // 12.123V
        uint8_t mv = abs(voltage_mv) % 1000;
        return snprintf(s, n, "%1d.%03dV", voltage_mv / 1000, mv);
    }

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

static inline uint8_t print_current(char *s, uint8_t n, int32_t current_100uA, bool large) {
    if (large) {
        if (abs(current_100uA) < 1000*10) { // < 1A
            // 123.4mA
            return snprintf(s, n, "%1d.%01dmA", current_100uA / 10, abs(current_100uA) % 10);
        }
        // 12.345A
        // 1.234A
        current_100uA /= 10; // Convert to mA
        uint16_t ma = abs(current_100uA) % 1000;
        current_100uA /= 1000; // Convert to amps
        return snprintf(s, n, "%1d.%03dA", current_100uA, ma);
    }
    if (abs(current_100uA) < 1000) { // < 100mA
        // 12.3mA
        return snprintf(s, n, "%2d.%01dmA", current_100uA / 10, abs(current_100uA) % 10);
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
    char s[17*5];
    char *sp = s;
    uint8_t n = sizeof(s);
    uint8_t w = 0;

    for (uint8_t i = 0; i < 5; i++) {
        w = print_voltage(sp, n, st.screen0.voltage_mv[i], false);
        sp += w;
        n -= w;
        
        if (n > 1) {
            *(sp++) = ' ';
            n -= 1;
        }

        w = print_current(sp, n, st.screen0.current_100uA[i], false);
        sp += w;
        n -= w;
        
        if (st.screen0.overflow[i]) {
            if (n > 2) {
                *(sp++) = ' ';
                *(sp++) = '*';
                n -= 2;
            }
        }

        if (n > 1 && i != 4) {
            *(sp++) = '\n';
            n -= 1;
        }

    }

    if (n <= 1) {
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

static void draw_screen1_5() {
    uint8_t i = current_screen - 1; // 0-4
    if (i > 4) return;

    for (uint8_t j = 0; j < i; j++) {
        // Draw dots before icon
        ssd1306_draw_square(&disp, j*12+5, 7, 2, 2);
    }
    ssd1306_draw_char_with_font(&disp, i*12, 0, 1, Chan_Icons, 'A' + i); // Move icon along with index
    for (uint8_t j = i+1; j < 5; j++) {
        // Draw dots after icon
        ssd1306_draw_square(&disp, j*12-12+32+5, 7, 2, 2);
    }


    if (st.screen0.on_state) {
        ssd1306_draw_char_with_font(&disp, 128-On_Icon[1]-2, 2, 1, On_Icon, 'A');
    }

    char s[32];
    uint8_t n = sizeof(s);

    uint8_t w = print_voltage(s, n, st.screen0.voltage_mv[i], true);
    n -= w;

    if (st.screen0.overflow[i]) {
        if (n > 2) {
            s[w++] = ' ';
            s[w++] = '*';
            n -= 2;
        }
    }
    
    if (n > 1) {
        s[w++] = '\n';
        n -= 1;
    }

    n -= print_current(&s[w], n, st.screen0.current_100uA[i], true);

    if (n <= 1) {
        printf("Error: String overflow rendering screen 1-5");
    }

    draw_string(&disp, 1, 18, 2, Pix32_Font, s);
}

// static int anim_x = 0;
// static int anim_y = 0;

// static void draw_anim() {
//     ssd1306_draw_square(&disp, anim_x, 0, 2, 64);
//     anim_x = (anim_x + 1) % 128;
//     ssd1306_draw_square(&disp, 0, anim_y, 128, 2);
//     anim_y = (anim_y + 1) % 64;
// }

static void draw_settings_menu() {
    char s[200];
    uint8_t i = 0;

    if (!st.settings.is_open) {
        draw_string(&disp, 64-(8*Pix32_Font[1]), 32-Pix32_Font[0], 2, Pix32_Font, "Settings");
        return;
    }

    if (st.settings.sel && st.settings.index == 0) {
        draw_string(&disp, 64-(9*Pix32_Font[1]), 32-(2*Pix32_Font[0]), 2, Pix32_Font, " Restore\nDefaults?");
        return;
    }

    // Keep selected setting off the last row (unless it is the last)
    uint8_t si = 0;
    if (NUM_SETTINGS > 4) {
        if (st.settings.index >= NUM_SETTINGS-1) {
            si = NUM_SETTINGS-4;
        } else if (st.settings.index > 3) {
            si = st.settings.index-3;
        }
    }
    
    for (uint8_t j = si; j < si + 5 && j <= NUM_SETTINGS; j++) {
        char sel = st.settings.index == j ? '*' : ' ';
        const char *inv_c = st.settings.index == j && st.settings.sel ? "\a" : "";

        if (j != si && i+1 < sizeof s) {
            s[i++] = '\n';
            s[i+1] = 0;
        }
        
        switch (j) {
            // Width can fit 21 chars (13 max chars for text after sel)
            case 0:
                i += snprintf(&s[i], (sizeof s) - i, "%c     Settings", sel);
                break;
            case SETTING_CONTRAST:
                i += snprintf(&s[i], (sizeof s) - i, "%c Disp Contrast", sel);
                // Draw in inverted font if selected
                i += snprintf(&s[i], (sizeof s) - i, " %s<%3d>%s", inv_c, st.settings.tmp.disp_contrast / CONTRAST_STEP, inv_c);
                break;
            case SETTING_STARTUP_STATE:
                i += snprintf(&s[i], (sizeof s) - i, "%c Startup State", sel);
                // Draw in inverted font if selected
                i += snprintf(&s[i], (sizeof s) - i, " %s< %d >%s", inv_c, st.settings.tmp.startup_state, inv_c);
                break;
            case SETTING_EN_LOGGING:
                i += snprintf(&s[i], (sizeof s) - i, "%c Logging Mode ", sel);
                // Draw in inverted font if selected
                i += snprintf(&s[i], (sizeof s) - i, " %s< %d >%s", inv_c, st.settings.tmp.uart_logging, inv_c);
                break;
            default:
                // i += snprintf(&s[i], (sizeof s) - i, "\n%c", sel);
                i += snprintf(&s[i], (sizeof s) - i, "%c%13d", sel, j);
                i += snprintf(&s[i], (sizeof s) - i, " %s     %s", inv_c, inv_c);
        }
    }

    if (i+1 >= sizeof s) {
        printf("Error: String overflow rendering settings menu");
    }

    draw_string_with_inverts(&disp, 1, 2, 1, s);
}

void open_settings(void) {
    if (st.settings.is_open) return;
    st.settings.is_open = true;
    // Load settings into tmp copy
    memcpy(&st.settings.tmp, &settings, sizeof settings);
    // Go to top of list
    st.settings.index = 0;
    st.settings.sel = false;
    dirty = true;
}

bool update_settings_menu(uint8_t btn1, uint8_t btn2) {
    uint8_t tmp_contrast = st.settings.tmp.disp_contrast;
    if (!st.settings.is_open) return true;

    if (st.settings.sel) {
        if (btn1 == SHORT_PRESS && btn2 == SHORT_PRESS) {
            // Do nothing
        } else if (btn1 == SHORT_PRESS || btn2 == SHORT_PRESS) {
            bool up = btn1 == SHORT_PRESS;
            switch (st.settings.index) {
                case 0:
                    // Allow short presses to cancel restore defaults
                    st.settings.sel = false; // Cancel
                    break;
                case SETTING_CONTRAST:
                    tmp_contrast -= tmp_contrast % CONTRAST_STEP; // Floor to nearest multiple
                    if (tmp_contrast == 0 && !up) tmp_contrast = 0xff;
                    else if (tmp_contrast == 0xff && up) tmp_contrast = 0;
                    else tmp_contrast += up ? CONTRAST_STEP : -CONTRAST_STEP;
                    st.settings.tmp.disp_contrast = tmp_contrast;
                    st.settings.modified = st.settings.tmp.disp_contrast != settings.disp_contrast;
                    printf("Contrast set to: 0x%02x\n", tmp_contrast);

                    if (tmp_contrast == 0) tmp_contrast = 0x80; // Don't go fully invisible
                    ssd1306_contrast(&disp, tmp_contrast);
                    break;
                case SETTING_STARTUP_STATE:
                    st.settings.tmp.startup_state ^= 1;
                    st.settings.modified = st.settings.tmp.startup_state != settings.startup_state;
                    break;
                case SETTING_EN_LOGGING:
                    st.settings.tmp.uart_logging += up ? 1 : -1;
                    if (st.settings.tmp.uart_logging > LOGGING_CSV) {
                        st.settings.tmp.uart_logging = up ? 0 : LOGGING_CSV;
                    }
                    st.settings.modified = st.settings.tmp.uart_logging != settings.uart_logging;
                    break;
            }
        } else if (btn1 == LONG_PRESS) {
            st.settings.sel = false; // Cancel
            // Load current value
            switch (st.settings.index) {
                case SETTING_CONTRAST:
                    st.settings.tmp.disp_contrast = settings.disp_contrast;
                    ssd1306_contrast(&disp, 0xff); // Go back to full contrast
                    break;
                case SETTING_STARTUP_STATE:
                    st.settings.tmp.startup_state = settings.startup_state;
                    break;
                case SETTING_EN_LOGGING:
                    st.settings.tmp.uart_logging = settings.uart_logging;
                    break;
            }
        } else if (btn2 == LONG_PRESS) {
            st.settings.sel = false; // Save
            if (st.settings.index == SETTING_CONTRAST) {
                ssd1306_contrast(&disp, 0xff); // Go back to full contrast
            }
            show_popup_centered(63, 31, 2, 500, 5, 1, "Saved");
            if (st.settings.index == 0) {
                // Restore defaults
                load_default_settings();
                memcpy(&st.settings.tmp, &settings, sizeof settings);
                save_settings();
                return true; // Exit settings
            } else if (st.settings.modified) {
                memcpy(&settings, &st.settings.tmp, sizeof settings);
                save_settings();
            }
        }
    } else {
        if (btn1 == SHORT_PRESS && btn2 == SHORT_PRESS) {
            // Do nothing
        } else if (btn1 == SHORT_PRESS) {
            if (st.settings.index == 0) st.settings.index = NUM_SETTINGS;
            else st.settings.index -= 1;
        } else if (btn2 == SHORT_PRESS) {
            st.settings.index = (st.settings.index + 1) % (NUM_SETTINGS+1);
        } else if (btn1 == LONG_PRESS) {
            return true;
        } else if (btn2 == LONG_PRESS) {
            st.settings.sel = true; // Select item
            if (st.settings.index == SETTING_CONTRAST) {
                if (tmp_contrast == 0) tmp_contrast = 0x80; // Don't go fully invisible
                ssd1306_contrast(&disp, tmp_contrast);
            }
            st.settings.modified = false;
        }
    }

    dirty |= btn1 || btn2; // Simple way to update the dirty flag

    return false;
}
