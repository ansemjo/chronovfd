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
  
  // init the vfd driver
  const vfd_handle_t vfd = vfd_init(NULL, "ivl2-7/5");

  // test, display 88:88 statically
  vfd_data(&vfd, 0xffff);
  ESP_LOGI("main", "display test, full on");

  vTaskDelay(1000 / portTICK_RATE_MS);

  vfd_mux_init();
  ESP_LOGI("main", "begin digit multiplexing");
  vfd_rawbuf[0] = segment_lookup('H');
  vfd_rawbuf[1] = segment_lookup('E');
  vfd_rawbuf[3] = segment_lookup('L');
  vfd_rawbuf[4] = segment_lookup('O');
  ESP_LOGI("main", "HELO");

}

