#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_types.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "driver/ledc.h"
#include "driver/adc.h"

#include "vfddriver.h"
#include "segments.h"

// turn on the usr led, yay
#define LED 5
void led_init() {
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_level(LED, 1);
}

uint16_t sma_buf[20];
unsigned sma_n = 20;
unsigned sma_pos = 0;
void sma_init(uint16_t init) {
  for (int i = 0; i < sma_n; i++) {
    sma_buf[i] = init;
  };
  sma_pos = 1;
}
uint16_t sma_next(uint16_t next) {
  sma_buf[sma_pos] = next;
  uint32_t sum = 0;
  for (int i = 0; i < sma_n; i++) {
    sum += sma_buf[i];
  };
  sma_pos = (sma_pos + 1) % sma_n;
  return sum / sma_n;
}

// test pwm dimming of filament via power supply shutdown pin
void filament_dimmer_init() {

  ledc_timer_config_t pwmcfg = {
    .freq_hz = 60000,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .duty_resolution = 8,
    .clk_cfg = LEDC_AUTO_CLK,
  };
  ledc_timer_config(&pwmcfg);
  ledc_channel_config_t pwmchan = {
    .channel = LEDC_CHANNEL_0,
    .duty = 0,
    .gpio_num = VFD_PIN_FILSHDN,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .hpoint = 0,
    .timer_sel = LEDC_TIMER_0,
  };
  ledc_channel_config(&pwmchan);
  ESP_LOGI("pwm", "initialized");

}

// taken from arduino
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long capped(long a, long b) {
  return (a > b) ? b : a;
}

// display the current runtime in MM:SS on display
void runtime(vfd_handle_t *vfd) {

  static time_t now;
  static struct tm *timeinfo;
  static char timebuf[32];
  
  time(&now);
  timeinfo = localtime(&now);
  strftime(timebuf, sizeof(timebuf), "%M:%S", timeinfo);
  vfd_text(vfd, timebuf);
  
}

void app_main() {

  esp_err_t err;
    
  // init default nonvolatile storage
  err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // led_init();
  vfd_handle_t* vfd = chronovfd_init();

  filament_dimmer_init();

  // Pin 4, SENSOR_VP == GPIO 36 == ADC1 Channel 0
  gpio_set_direction(ADC1_CHANNEL_0_GPIO_NUM, GPIO_MODE_INPUT);
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);
  int value;
  int duty;
  char adcbuf[32];
  sma_init(adc1_get_raw(ADC1_CHANNEL_0));

  while (true) {

    value = sma_next(adc1_get_raw(ADC1_CHANNEL_0));
    duty = 190 - capped(map(value, 0, 400, 0, 190), 190);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    
    // snprintf(adcbuf, sizeof(adcbuf), "%04d", duty);
    // for (int c = 4; c >= 2; c--) {
    //   adcbuf[c+1] = adcbuf[c];
    // }
    // vfd_text(vfd, adcbuf);

    runtime(vfd);
    vTaskDelay(100 / portTICK_RATE_MS);
  }


}



// --------------------------- snippets -------------------------------

void filament_dimmer_fader() {

  ledc_fade_func_install(0);
  vTaskDelay(2000 / portTICK_RATE_MS);
  while (1) {
    ESP_LOGI("pwm", "dimming down to 255");
    ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 255, 5000, LEDC_FADE_WAIT_DONE);
    vTaskDelay(2000 / portTICK_RATE_MS);
    ESP_LOGI("pwm", "dimming up to 160");
    ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0, 5000, LEDC_FADE_WAIT_DONE);
    vTaskDelay(2000 / portTICK_RATE_MS);
  };

}