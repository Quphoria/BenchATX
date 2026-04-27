#pragma once

#include <stdint.h>
#include <stdbool.h>

// Return fake values for testing without a sensor connected
#define DUMMY_SENSOR

#define NUM_POWER_SENSORS 5

void init_power_sensors(void);
float power_sens_get_voltage(uint8_t sensor);
float power_sens_get_current(uint8_t sensor);
float power_sens_get_power(uint8_t sensor);
bool power_sens_get_overflow(uint8_t sensor);

