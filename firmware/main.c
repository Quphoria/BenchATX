#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/unique_id.h"
// To allow reset to bootloader
#include "pico/bootrom.h"
#include "pico/time.h"
#include "hardware/i2c.h"
#include "hardware/watchdog.h"

#include "buttons.h"
#include "display.h"
#include "power_sensor.h"
#include "status_led.h"

#define WATCHDOG_TIMEOUT_MS 2000
#define WATCHDOG_UPDATE_TIMER_MS 500

#define BOARD_ID_STR_LEN (2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1)

static bool initialized = false;
static bool alive = true;

bool watchdog_update_timer_callback(struct repeating_timer *t) {
    if (!initialized) return true;
    if (!alive) return true;
    alive = false;

    watchdog_update();

    return true;
}

static struct repeating_timer watchdog_update_timer;

void setup_i2c(void) {
    i2c_init(i2c0, 400 * 1000); // 400kHz I2C Fast Mode
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
static const uint PS_ON_PIN = 8;
static const uint PWR_OK_PIN = 9;

static inline void init_btns() {
    for (uint8_t i = 0; i < NUM_BTNS; i++) {
        gpio_init(btn_status.pins[i]);
        gpio_set_dir(btn_status.pins[i], GPIO_IN);
        gpio_set_pulls(btn_status.pins[i], true, false); // Enable pullup
        btn_status.prev_state[i] = true;
        btn_status.debounce_delay[i] = get_absolute_time();
    }
}

static inline uint8_t check_btn(uint8_t index) {
    if (index >= 2) return 0;

    absolute_time_t t = get_absolute_time();

    // Check if delay expiry is in the future
    if (absolute_time_diff_us(t, btn_status.debounce_delay[index]) >= 0) return 0;

    // Reuse debounce delay timeout time, just subtract the debounce delay
    bool long_press = !btn_status.prev_state[index] && absolute_time_diff_us(btn_status.debounce_delay[index], t) > 1000*(LONG_PRESS_MS-DEBOUNCE_DELAY_MS);
    if (long_press && !btn_status.long_press[index]) {
        btn_status.long_press[index] = true;
        return LONG_PRESS;
    }

    bool new_state = gpio_get(btn_status.pins[index]);
    if (btn_status.prev_state[index] != new_state) {
        btn_status.prev_state[index] = new_state;
        btn_status.debounce_delay[index] = make_timeout_time_ms(DEBOUNCE_DELAY_MS); // 50ms debounce
        btn_status.long_press[index] = false;
        if (new_state == 1 && !long_press) { // 0 -> 1
            return SHORT_PRESS;
        }
    }
    return 0;
}

int main() {
    stdio_init_all();

    gpio_init(PS_ON_PIN);
    gpio_set_dir(PS_ON_PIN, GPIO_OUT);
    gpio_put(PS_ON_PIN, false);

    gpio_init(PWR_OK_PIN);
    gpio_set_dir(PWR_OK_PIN, GPIO_IN);

    printf("\n\n\n\n\n");
    printf("BenchATX\n");

    if (watchdog_caused_reboot() &&
        watchdog_enable_caused_reboot()) printf("Rebooted by Watchdog!\n");

    pico_unique_board_id_t board_id;
    char board_id_str[BOARD_ID_STR_LEN] = "";
    pico_get_unique_board_id(&board_id);
    pico_get_unique_board_id_string(board_id_str, BOARD_ID_STR_LEN);
    printf("Board ID: %s\n", board_id_str);

    add_repeating_timer_ms(WATCHDOG_UPDATE_TIMER_MS, watchdog_update_timer_callback, NULL, &watchdog_update_timer);

    sleep_ms(500);

    // Start watchdog
    watchdog_enable(WATCHDOG_TIMEOUT_MS, true); // Pause watchdog on debug

    printf("Initializing peripherals...\n");

    setup_i2c();
    init_btns();
    init_status_led(pio0);
    init_power_sensors();
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

    uint8_t current_screen = 0;
    bool is_on = false;
    absolute_time_t measure_delay = get_absolute_time();

    set_status_led(0x00, 0x60, 0x04, false);
    initialized = true;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    int led = 0;
    while (true) {
        alive = true;

        led = ~led;
        gpio_put(LED_PIN, led);
        // sleep_ms(50);

        if (absolute_time_diff_us(get_absolute_time(), measure_delay) < 0) {
            measure_delay = make_timeout_time_ms(100); // Measure every 100ms

            printf("\n");

            for (uint8_t i = 0; i < NUM_POWER_SENSORS; i++) {
                // Address order is backwards
                float v = power_sens_get_voltage(NUM_POWER_SENSORS-1-i);
                float c = power_sens_get_current(NUM_POWER_SENSORS-1-i);

                printf("[CH %0d] Voltage: %6.3f V, Current %.1f mA\n", i, v, c*1000);
                
                update_voltage(i, v*1000);
                update_current(i, c*10000);
            }
        }

        uint8_t btn1 = check_btn(0);
        uint8_t btn2 = check_btn(1);

        if (btn1 == SHORT_PRESS && btn2 != SHORT_PRESS) {
            if (current_screen == 0) {
                current_screen = NUM_SCREENS-1;
            } else {
                current_screen -= 1;
            }
            set_current_screen(current_screen);
        } else if (btn2 == SHORT_PRESS) {
            current_screen = (current_screen + 1) % NUM_SCREENS;
            set_current_screen(current_screen);
        } else if (btn2 == LONG_PRESS) {
            is_on = !is_on;
            printf("Turning PSU %s\n", is_on ? "ON" : "OFF");
            update_on_state(is_on);
            gpio_put(PS_ON_PIN, is_on);
        }

        update_pwr_ok(gpio_get(PWR_OK_PIN));

        refresh_display();
    }
}
