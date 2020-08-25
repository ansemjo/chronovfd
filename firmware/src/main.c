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
#include "ambientlight.h"

// turn on the usr led, yay
#define LED 5
void led_init() {
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_level(LED, 1);
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

  vfd_handle_t* vfd = chronovfd_init();
  
  // Pin 4, SENSOR_VP == GPIO 36 == ADC1 Channel 0
  ambientlight_dimmer_init(ADC1_CHANNEL_0, vfd->pin.fil_shdn);

  while (true) {
    runtime(vfd);
    vTaskDelay(100 / portTICK_RATE_MS);
  }

}