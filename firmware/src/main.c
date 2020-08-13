#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

// define interface to the hv5812
#define HV_SPI  HSPI_HOST
#define DMA_CHAN        2
#define HV_PIN_ENABLE  GPIO_NUM_19
#define HV_PIN_CLOCK   GPIO_NUM_25
#define HV_PIN_DATA    GPIO_NUM_26
#define HV_PIN_STROBE  GPIO_NUM_27
#define HV_PIN_BLANK   GPIO_NUM_18
#define HV_PIN_FILSHDN GPIO_NUM_17
#define HV_PIN_HVSHDN  GPIO_NUM_16

// toggle the strobe line to display transmitted digit
void hv_spi_pre_strobe(spi_transaction_t *t) {
  gpio_set_level(HV_PIN_STROBE, 0);
}
void hv_spi_post_strobe(spi_transaction_t *t) {
  gpio_set_level(HV_PIN_STROBE, 1);
}

// send data
void hv_data(spi_device_handle_t hv, const uint8_t *data, int len) {

  esp_err_t ret;
  spi_transaction_t t;
  if (len == 0) return;

  memset(&t, 0, sizeof(t));
  t.length = len * 8;
  t.tx_buffer = data;
  t.user = (void*)1; // wtf?
  ret = spi_device_polling_transmit(hv, &t);
  ESP_ERROR_CHECK(ret);

}

// initialize hv to display all segments
void hv_init(spi_device_handle_t hv) {

  // configure aux gpios for display
  gpio_set_direction(HV_PIN_ENABLE,  GPIO_MODE_OUTPUT);
  gpio_set_direction(HV_PIN_STROBE,  GPIO_MODE_OUTPUT);
  gpio_set_direction(HV_PIN_BLANK,   GPIO_MODE_OUTPUT);
  gpio_set_direction(HV_PIN_FILSHDN, GPIO_MODE_OUTPUT);
  gpio_set_direction(HV_PIN_HVSHDN,  GPIO_MODE_OUTPUT);
  gpio_set_level(HV_PIN_BLANK, 0);
  gpio_set_level(HV_PIN_FILSHDN, 0);
  gpio_set_level(HV_PIN_HVSHDN, 1);
  gpio_set_level(HV_PIN_ENABLE, 0);
  // vTaskDelay(100 / portTICK_RATE_MS);
  // gpio_set_level(HV_PIN_FILSHDN, 1);

  const uint8_t fullon[2] = {0xff, 0xff};
  hv_data(hv, fullon, 2);

}

#define LED 5
void led_init() {
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_level(LED, 1);
}

void app_main() {

  esp_err_t ret;
    
  // init default nonvolatile storage
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // init spi line to hv shifter
  spi_device_handle_t hv;
  spi_bus_config_t hvbuscfg = {
    .sclk_io_num = HV_PIN_CLOCK,
    .mosi_io_num = HV_PIN_DATA,
    .miso_io_num = -1,
    .quadhd_io_num = -1,
    .quadwp_io_num = -1,
  };
  spi_device_interface_config_t hvdevcfg = {
    .clock_speed_hz = 5*1000*1000, // maximum is 5MHz @ 125°C, 5V
    .mode = 0,
    .pre_cb  = hv_spi_pre_strobe,
    .post_cb = hv_spi_post_strobe,
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .duty_cycle_pos = 128,
    .spics_io_num = -1,
    .queue_size=1,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(HV_SPI, &hvbuscfg, 1));
  ESP_LOGI("hv5812", "bus configured");
  ESP_ERROR_CHECK(spi_bus_add_device(HV_SPI, &hvdevcfg, &hv));
  ESP_LOGI("hv5812", "device configured");
  hv_init(hv);

  led_init();
  ESP_LOGI("LED", "turned on");


  //wifi_init_softap();

}

