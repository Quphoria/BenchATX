/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
// To allow reset to bootloader
#include "pico/bootrom.h"
#include "hardware/i2c.h"

#include "Pix32_Font.h"
#include "On_Icon.h"

#include "ssd1306.h"

void setup_oled(void) {
    i2c_init(i2c0, 400000);
    gpio_set_function(12, GPIO_FUNC_I2C);
    gpio_set_function(13, GPIO_FUNC_I2C);
    gpio_pull_up(12);
    gpio_pull_up(13);
}

static void enter_bootloader(void) {
    // Enter the bootloader
    reset_usb_boot(0,0); // No activity led
}

static const uint LED_PIN = 25;
static const uint BTN1_PIN = 6;
static const uint BTN2_PIN = 7;

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

void draw_screen1(ssd1306_t *p, bool on) {
    ssd1306_clear(p);

    draw_string(p, 1, 2, 1, Pix32_Font, 
        "3.31V 0.123A\n"
        "5.01V 1.547A\n"
        "4.98V 6.819A\n"
        "12.0V  2.3mA\n"
        "11.8V 59.7mA"
    );

    if (on) {
        ssd1306_draw_char_with_font(p, 128-(On_Icon[1]*2)-2, 32-On_Icon[0], 2, On_Icon, 'A');
    }

    ssd1306_show(p);
}

int main() {
    stdio_init_all();

    setup_oled();

    ssd1306_t disp;

    disp.external_vcc=false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c0);
    /* Possible changes to init cmds
    
    SET_CONTRAST,
    0xCF, // From https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.cpp#L598
    ...
    SET_VCOM_DESEL,
    0x40, // From https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.cpp#L618
    */

    // ssd1306_clear(&disp);
    // ssd1306_draw_square(&disp, 0, 0, 128, 64);
    // ssd1306_show(&disp);
   
    draw_screen1(&disp, false);
    
    gpio_init(BTN1_PIN);
    gpio_init(BTN2_PIN);
    gpio_set_dir(BTN1_PIN, GPIO_IN);
    gpio_set_dir(BTN2_PIN, GPIO_IN);
    gpio_set_pulls(BTN1_PIN, true, false); // Enable pullup
    gpio_set_pulls(BTN2_PIN, true, false); // Enable pullup

    int x = 0;
    int y = 0;

    bool show_anim = false;

    bool btn1_prev = true;
    bool btn2_prev = true;
    bool is_on = false;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    int led = 0;
    while (true) {
        led = ~led;
        gpio_put(LED_PIN, led);
        // sleep_ms(50);

        if (gpio_get(BTN1_PIN) != btn1_prev) {
            if (btn1_prev) { // 1 -> 0
                show_anim = !show_anim;
                if (!show_anim) {
                    draw_screen1(&disp, is_on);
                }
            }
            btn1_prev = !btn1_prev;
            sleep_ms(50); // Lazy Debounce
        }
        
        if (gpio_get(BTN2_PIN) != btn2_prev) {
            if (btn2_prev) { // 1 -> 0
                is_on = !is_on;
                if (!show_anim) {
                    draw_screen1(&disp, is_on);
                }
            }
            btn2_prev = !btn2_prev;
            sleep_ms(50); // Lazy Debounce
        }

        if (show_anim) {
            ssd1306_clear(&disp);
            ssd1306_draw_square(&disp, x, 0, 2, 64);
            x = (x + 1) % 128;
            ssd1306_draw_square(&disp, 0, y, 128, 2);
            y = (y + 1) % 64;
            ssd1306_show(&disp);
        }
    }
}
