#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"

#include "ambientlight.h"


// ---------------- simple moving average ----------------
//   for dampening of the ambient light sensor readings

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

#define TAG "ambientlight"

// struct holding the ledc-controlled pwm variables
typedef struct ambientlight_t {
  ledc_timer_t timer;
  ledc_mode_t mode;
  ledc_channel_t channel;
  adc1_channel_t adc;
  moving_average_t avg;
  gpio_num_t pin;
} ambientlight_t;

// global variable for the filament dimmer on chronovfd
static ambientlight_t dimmer;

// mapping an adc reading to pwm duty cycle, lower reading -> higher shutdown duty
unsigned ambientmap(unsigned value, unsigned reading_max, unsigned duty_min) {
  if (value >= reading_max) return 0;
  return ((reading_max - value) * 1.0 / reading_max) * duty_min;
}

void ambientlight_task(void *arg) {
  uint16_t value;
  uint16_t duty;
  
  do {
    // fill average with initial value once
    uint16_t first = adc1_get_raw(dimmer.adc);
    for (int i = 0; i < AVERAGE_POINTS; i++) {
      dimmer.avg.buf[i] = first;
    };
  } while (0);

  for (;;) {
    value = moving_average(&dimmer.avg, adc1_get_raw(dimmer.adc));
    duty = ambientmap(value, 600, 200);
    ledc_set_duty(dimmer.mode, dimmer.channel, duty);
    ledc_update_duty(dimmer.mode, dimmer.channel);
    vTaskDelay(100 / portTICK_RATE_MS);
  }
}

void ambientlight_init(adc1_channel_t sensor, gpio_num_t filshdn, TaskHandle_t *task) {

  // set variables in global struct
  dimmer.mode = LEDC_HIGH_SPEED_MODE;
  dimmer.timer = LEDC_TIMER_0;
  dimmer.channel = LEDC_CHANNEL_0;
  dimmer.adc = sensor;
  dimmer.pin = filshdn;

  ledc_timer_config_t pwmcfg = {
    .freq_hz = 60000,
    .speed_mode = dimmer.mode,
    .timer_num = dimmer.timer,
    .duty_resolution = 8,
    .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&pwmcfg));
  
  ledc_channel_config_t pwmchan = {
    .channel = dimmer.channel,
    .duty = 0,
    .gpio_num = dimmer.pin,
    .speed_mode = dimmer.mode,
    .hpoint = 0,
    .timer_sel = dimmer.timer,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&pwmchan));
  ESP_LOGI(TAG, "pwm initialized");

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(dimmer.adc, ADC_ATTEN_DB_0);
  xTaskCreate(ambientlight_task, TAG, 1024, NULL, 10, task);
  ESP_LOGI(TAG, "adc configured & task created");

}


