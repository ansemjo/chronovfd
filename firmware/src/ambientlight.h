#ifndef _AMBIENTLIGHT_H_
#define _AMBIENTLIGHT_H_

#include <stdint.h>
#include <stdlib.h>

#include "driver/ledc.h"
#include "driver/adc.h"


void ambientlight_dimmer_init(adc1_channel_t sensor, gpio_num_t filshdn);

#endif // _AMBIENTLIGHT_H_