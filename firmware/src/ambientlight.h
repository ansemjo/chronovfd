#pragma once

#include <driver/gpio.h>
#include <driver/adc.h>

// ---------------- simple moving average ----------------

// how many points to hold in a moving average buffer
#define AVERAGE_POINTS 20

// struct for the state of a simple moving average calculation
typedef struct moving_average_t {
  uint16_t buf[AVERAGE_POINTS];
  int pos;
} moving_average_t;

uint16_t moving_average(moving_average_t *avg, uint16_t next);


// ---------------- pwm-controlled filament dimmer ----------------

// photodiode on pin 4 / gpio 36 == SENSOR_VP == ADC1 Channel 0
#define PHOTODIODE ADC1_CHANNEL_0

void ambientlight_init(adc1_channel_t sensor, gpio_num_t filshdn, TaskHandle_t *task);
