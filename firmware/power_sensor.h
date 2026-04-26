#pragma once

#include <stdint.h>

#define NUM_POWER_SENSORS 5

void init_power_sensors(void);
float power_sens_get_voltage(uint8_t sensor);
float power_sens_get_current(uint8_t sensor);
float power_sens_get_power(uint8_t sensor);

