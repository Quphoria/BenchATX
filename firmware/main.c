#include "pico/stdlib.h"
// To allow reset to bootloader
#include "pico/bootrom.h"
#include "hardware/i2c.h"

#include "display.h"

void setup_i2c(void) {
    i2c_init(i2c0, 400000); // 400kHz I2C Fast Mode
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

    setup_i2c();
    init_display();

    update_voltage(0, 3.31*1000);
    update_voltage(1, 5.01*1000);
    update_voltage(2, 4.98*1000);
    update_voltage(3, 12.0*1000);
    update_voltage(4, 11.8*1000);

    update_current(0, 0.123*10000);
    update_current(1, 1.547*10000);
    update_current(2, 6.819*10000);
    update_current(3, 0.0023*10000);
    update_current(4, 0.0597*10000);

    update_on_state(false);
   
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
                set_current_screen(show_anim ? 10 : 0);
            }
            btn1_prev = !btn1_prev;
            sleep_ms(50); // Lazy Debounce
        }
        
        if (gpio_get(BTN2_PIN) != btn2_prev) {
            if (btn2_prev) { // 1 -> 0
                is_on = !is_on;
                update_on_state(is_on);
            }
            btn2_prev = !btn2_prev;
            sleep_ms(50); // Lazy Debounce
        }

        refresh_display();
    }
}
