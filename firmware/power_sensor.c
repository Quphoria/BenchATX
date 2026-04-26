#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "power_sensor.h"

#define POWER_SENS_I2C i2c0
#define POWER_SENS_START_ADDR 0x40

#define PWR_REG_CONFIG  0
#define PWR_REG_SHUNT_V 1
#define PWR_REG_BUS_V   2
#define PWR_REG_POWER   3
#define PWR_REG_CURRENT 4
#define PWR_REG_CALIBR  5

#define PWR_CFG_RESET   (1<<15)
#define PWR_CFG_BRNG    (0<<13) // Bus Voltage Range: 16V FSR
#define PWR_CFG_PG      (0b01<<11) // +-80mV Shunt Voltage
#define PWR_CFG_BADC    (0b1011<<7) // Bus ADC Res/Avg 8 samples = 4.26ms conversion time 
#define PWR_CFG_SADC    (0b1011<<3) // Shunt ADC Res/Avg 8 samples = 4.26ms conversion time 
#define PWR_CFG_MODE    (0b111) // Mode: Shunt and bus, continuous

// Cal = 0.04096 / (Current_LSB * Rshunt)
// Current_LSB = (Max Expected Current / 2^15)
// RShunt = 0.005
// Max Expected current = 20A
// Cal = 13421
// Shunt Voltage Register Max = 8000
// Giving us (8000 * 16777) / 4096 = 32767
// 32767 * Current_LSB = 19.999A

#define MAX_EXPECTED_CURRENT 20
#define CURRENT_LSB (MAX_EXPECTED_CURRENT / pow(2, 15))
#define POWER_LSB (20*CURRENT_LSB)
#define PWR_CALIBRATION_VALUE 16777

void power_sens_write_reg(uint8_t sensor, uint8_t reg, uint16_t val);
uint16_t power_sens_read_reg(uint8_t sensor, uint8_t reg);

void init_power_sensors(void) {
    return;
    for (uint8_t i = 0; i < NUM_POWER_SENSORS; i++) {
        // First reset chip
        power_sens_write_reg(i, PWR_REG_CONFIG, PWR_CFG_RESET);
        // Now set config
        power_sens_write_reg(i, PWR_REG_CONFIG,
            PWR_CFG_BRNG | PWR_CFG_PG | PWR_CFG_BADC | PWR_CFG_SADC | PWR_CFG_MODE
        ); // Set MODE
    
        // Write calibration value
        power_sens_write_reg(i, PWR_REG_CALIBR, PWR_CALIBRATION_VALUE);
    }

    sleep_ms(5); // Wait for 5ms for first measurements to be made
}

static bool in_overflow[NUM_POWER_SENSORS] = {0};

bool power_sens_get_overflow(uint8_t sensor) {
    if (sensor >= NUM_POWER_SENSORS) panic("Invalid power sensor ID");
    return in_overflow[sensor];
}

float power_sens_get_voltage(uint8_t sensor) {
    return 1.234;
    int16_t value = power_sens_read_reg(sensor, PWR_REG_BUS_V);
    if (value & 0b1) printf("Warning: Power Sensor Overflow (Current/Power may be invalid)\n");
    in_overflow[sensor] = value & 0b1;
    // Don't check conversion bit as we are in continuous mode
    float voltage = (float)(value >> 3);
    voltage *= 0.004;
    return voltage;
}

float power_sens_get_current(uint8_t sensor) {
    return 12.3456;
    float current = (int16_t)power_sens_read_reg(sensor, PWR_REG_CURRENT);
    current *= CURRENT_LSB;
    return current;
}

float power_sens_get_power(uint8_t sensor) {
    float power = (int16_t)power_sens_read_reg(sensor, PWR_REG_POWER);
    power *= POWER_LSB;
    return power;
}

void power_sens_write_reg(uint8_t sensor, uint8_t reg, uint16_t val) {
    if (sensor >= NUM_POWER_SENSORS) panic("Invalid power sensor ID");
    uint8_t data[3] = {reg, (val >> 8) & 0xff, val & 0xff};

    int ret = i2c_write_blocking(POWER_SENS_I2C, POWER_SENS_START_ADDR + sensor, data, 3, false);
    if (ret == PICO_ERROR_GENERIC) panic("Failed to write to power sensor");
}

uint16_t power_sens_read_reg(uint8_t sensor, uint8_t reg) {
    if (sensor >= NUM_POWER_SENSORS) panic("Invalid power sensor ID");

    int ret = i2c_write_blocking(POWER_SENS_I2C, POWER_SENS_START_ADDR + sensor, &reg, 1, true);
    if (ret == PICO_ERROR_GENERIC) panic("Failed to write to power sensor");

    uint8_t data[2];

    ret = i2c_read_blocking(POWER_SENS_I2C, POWER_SENS_START_ADDR + sensor, data, 2, false);
    if (ret == PICO_ERROR_GENERIC) panic("Failed to read from power sensor");

    uint16_t value = (data[0] << 8) | data[1];
    return value;
}
