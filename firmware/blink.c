/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
// To allow reset to bootloader
#include "pico/bootrom.h"
#include "hardware/i2c.h"

#include "custom_font/SpleenTT.png.h"

#include "ssd1306.h"

void setup_oled(void) {
    i2c_init(i2c0, 50000);
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

int main() {
    stdio_init_all();

    setup_oled();

    ssd1306_t disp;
    disp.external_vcc=false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c0);
    ssd1306_clear(&disp);

    ssd1306_draw_string_with_font(&disp, 8, 8, 1, font_spleentt, "Hello");
    ssd1306_draw_string_with_font(&disp, 8, 8+8, 1, font_spleentt, "World!");
    ssd1306_draw_string_with_font(&disp, 8, 8+24, 1, font_spleentt, "12.3A 0.12A 15.6A 0.01A");
    ssd1306_draw_string_with_font(&disp, 8, 8+32, 1, font_spleentt, "12.3A 0.12A 15.6A 0.01A");
    
    // ssd1306_draw_square(&disp, 0, 0, 128, 64);
    ssd1306_show(&disp);
    
    gpio_init(BTN1_PIN);
    gpio_init(BTN2_PIN);
    gpio_set_dir(BTN1_PIN, GPIO_IN);
    gpio_set_dir(BTN2_PIN, GPIO_IN);
    gpio_set_pulls(BTN1_PIN, true, false); // Enable pullup
    gpio_set_pulls(BTN2_PIN, true, false); // Enable pullup

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
}
