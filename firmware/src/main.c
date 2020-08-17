#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#include "vfddriver.h"
#include "segments.h"

// turn on the usr led, yay
#define LED 5
void led_init() {
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_level(LED, 1);
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

  led_init();
  vfd_handle_t* vfd = chronovfd_init();

  // test pwm dimming of shdn pins
  vTaskDelay(2000 / portTICK_RATE_MS);
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

