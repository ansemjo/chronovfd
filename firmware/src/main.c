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

#include "ds1307.h"

#include "vfddriver.h"
#include "segments.h"
#include "ambientlight.h"

// turn on the usr led, yay
#define LED 5
void led_init() {
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_level(LED, 1);
}

void i2cscan(void *arg) {

  vfd_handle_t *vfd = arg;

  i2c_config_t i2cdrvcfg = {
    .mode = I2C_MODE_MASTER,
    .scl_io_num = 22,
    .scl_pullup_en = false,
    .sda_io_num = 21,
    .sda_pullup_en = false,
    .master.clk_speed = 100000,
  };
  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2cdrvcfg));

  i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
  
  while (true) {

    uint8_t address;
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    for (int i = 0; i < 128; i += 16) {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++) {
            fflush(stdout);
            address = i + j;
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, 0x1);
            i2c_master_stop(cmd);
            esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50 / portTICK_RATE_MS);
            i2c_cmd_link_delete(cmd);
            if (ret == ESP_OK) {
                printf("%02x ", address);
            } else if (ret == ESP_ERR_TIMEOUT) {
                printf("UU ");
            } else {
                printf("-- ");
            }
        }
        printf("\r\n\r\n");
    }

    vTaskDelay(2000 / portTICK_RATE_MS);
  }

}

// display the current runtime in MM:SS on display
void rtctime(void *arg) {

  vfd_handle_t *vfd = arg;
  
  i2c_dev_t rtc;
  memset(&rtc, 0, sizeof(i2c_dev_t));

  ESP_ERROR_CHECK(ds1307_init_desc(&rtc, I2C_NUM_0, 21, 22));
  ESP_LOGI("rtctime", "initialized ...");

  ESP_ERROR_CHECK(ds1307_start(&rtc, true));
  ESP_LOGI("rtctime", "rtc started ...");

  // time_t now;
  struct tm time;
  char timebuf[32];

  while (true) {

    // ERROR: hitting "(xQueueGenericReceive)- assert failed!" here
    // when trying to take the mutex
    if (ds1307_get_time(&rtc, &time) != ESP_OK) {
      printf("couldn't get time from rtc");
    } else {
      strftime(timebuf, sizeof(timebuf), "%M:%S", &time);
      printf("%s", timebuf);
      vfd_text(vfd, timebuf);
    };

    vTaskDelay(100 / portTICK_RATE_MS);
  }

  
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

  xTaskCreate(rtctime, "rtctime", 2048, vfd, 2, NULL);
  

}