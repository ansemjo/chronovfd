#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/ledc.h"
#include "driver/adc.h"

#include "ambientlight.h"


// ---------------- simple moving average ----------------
//   for dampening of the ambient light sensor readings

// how many points to hold in a moving average buffer
#define AVERAGE_POINTS 20

// struct for the state of a simple moving average calculation
typedef struct moving_average_t {
  uint16_t buf[AVERAGE_POINTS];
  unsigned pos;
} moving_average_t;

uint16_t moving_average(moving_average_t *avg, uint16_t next) {
  // TODO: there's a more efficient algorithm than summing every time
  avg->buf[avg->pos] = next;
  avg->pos = (avg->pos + 1) % AVERAGE_POINTS;
  uint32_t sum = 0;
  for (unsigned i = 0; i < AVERAGE_POINTS; i++) {
    sum += avg->buf[i];
  }
  return sum / AVERAGE_POINTS;
}

// ---------------- pwm-controlled filament dimmer ----------------

// struct holding the ledc-controlled pwm variables
typedef struct ambientlight_dimmer_t {
  ledc_timer_t timer;
  ledc_mode_t mode;
  ledc_channel_t channel;
  adc1_channel_t adc;
  moving_average_t avg;
  gpio_num_t pin;
} ambientlight_dimmer_t;

// global variable for the filament dimmer on chronovfd
ambientlight_dimmer_t filament;

// taken from arduino
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long capped(long a, long b) {
  return (a > b) ? b : a;
}

void ambientlight_dimmer(void *taskArg) {
  uint16_t value;
  uint16_t duty;
  while (true) {
    value = moving_average(&filament.avg, adc1_get_raw(filament.adc));
    duty = 190 - capped(map(value, 0, 400, 0, 190), 190);
    ledc_set_duty(filament.mode, filament.channel, duty);
    ledc_update_duty(filament.mode, filament.channel);
    vTaskDelay(100 / portTICK_RATE_MS);
  }
  vTaskDelete(NULL);
}

void ambientlight_dimmer_init(adc1_channel_t sensor, gpio_num_t filshdn) {

  // set variables in global struct
  filament.mode = LEDC_HIGH_SPEED_MODE;
  filament.timer = LEDC_TIMER_0;
  filament.channel = LEDC_CHANNEL_0;
  filament.adc = sensor;
  filament.pin = filshdn;

  ledc_timer_config_t pwmcfg = {
    .freq_hz = 60000,
    .speed_mode = filament.mode,
    .timer_num = filament.timer,
    .duty_resolution = 8,
    .clk_cfg = LEDC_AUTO_CLK,
  };
  ledc_timer_config(&pwmcfg);
  
  ledc_channel_config_t pwmchan = {
    .channel = filament.channel,
    .duty = 0,
    .gpio_num = filament.pin,
    .speed_mode = filament.mode,
    .hpoint = 0,
    .timer_sel = filament.timer,
  };
  ledc_channel_config(&pwmchan);
  ESP_LOGI("filament dimmer", "initialized");

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(filament.adc, ADC_ATTEN_DB_0);
  ESP_LOGI("filament dimmer", "adc configured");

  xTaskCreate(ambientlight_dimmer, "ambient light dimmer", 512, NULL, 10, NULL);
  ESP_LOGI("filament dimmer", "task created");

}


