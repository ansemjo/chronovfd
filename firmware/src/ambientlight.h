#pragma once

#include <driver/gpio.h>
#include <driver/adc.h>

void ambientlight_dimmer_init(adc1_channel_t sensor, gpio_num_t filshdn);
