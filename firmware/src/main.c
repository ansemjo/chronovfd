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

// define interface to the hv5812
#define VFD_SPI_HOST    HSPI_HOST
#define VFD_PIN_ENABLE  GPIO_NUM_19
#define VFD_PIN_CLOCK   GPIO_NUM_25
#define VFD_PIN_DATA    GPIO_NUM_26
#define VFD_PIN_STROBE  GPIO_NUM_27
#define VFD_PIN_BLANK   GPIO_NUM_18
#define VFD_PIN_FILSHDN GPIO_NUM_17
#define VFD_PIN_HVSHDN  GPIO_NUM_16

// #define HVMUX_GROUP   0
// #define HVMUX_ISRG    TIMERG0
// #define HVMUX_IDX     0
// #define HVMUX_DIVIDER 80

// turn on the usr led, yay
#define LED 5
void led_init() {
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  // gpio_set_level(LED, 1);
  // ESP_LOGI("LED", "turned on");
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
  vfd_pins_t pins = {
    .enable = VFD_PIN_ENABLE,
    .clock = VFD_PIN_CLOCK,
    .data = VFD_PIN_DATA,
    .strobe = VFD_PIN_STROBE,
    .blank = VFD_PIN_BLANK,
    .fil_shdn = VFD_PIN_FILSHDN,
    .hv_shdn = VFD_PIN_HVSHDN,
  };
  vfd_handle_t vfd = vfd_init(VFD_SPI_HOST, pins, "ivl2-7/5");

  // example, display HHHH statically
  uint16_t data = segment_lookup('H') | G1|G2|G4|G5;
  vfd_data(&vfd, data);


}

