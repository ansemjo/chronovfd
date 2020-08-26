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

// check if the rtc is configured as expected and return if everything was ok
bool rtc_config(i2c_dev_t *rtc) {

  bool ret = true;

  bool ok;
  ESP_ERROR_CHECK(ds1307_is_running(rtc, &ok));
  if (!ok) {
    ESP_LOGI("DS1338 RTC", "wasn't started. starting ...");
    ESP_ERROR_CHECK(ds1307_start(rtc, true));
    ret = false;
  }

  ds1307_squarewave_freq_t sqwf;
  ESP_ERROR_CHECK(ds1307_get_squarewave_freq(rtc, &sqwf));
  if (sqwf != DS1307_32768HZ) {
    ESP_LOGI("DS1338 RTC", "wrong SQWE frequency: RS = %x. setting 32.768 kHz ...", sqwf);
    ESP_ERROR_CHECK(ds1307_set_squarewave_freq(rtc, DS1307_32768HZ));
    ret = false;
  }

  ESP_ERROR_CHECK(ds1307_is_squarewave_enabled(rtc, &ok));
  if (!ok) {
    ESP_LOGI("DS1338 RTC", "SQWE output not enabled. enabling ...");
    ESP_ERROR_CHECK(ds1307_enable_squarewave(rtc, true));
    ret = false;
  }

  return ret;

}


// display the current runtime in MM:SS on display
void rtctime(void *arg) {

  vfd_handle_t *vfd = arg;

  i2c_dev_t rtc;
  memset(&rtc, 0, sizeof(i2c_dev_t));

  ESP_ERROR_CHECK(ds1307_init_desc(&rtc, I2C_NUM_0, 21, 22));
  ESP_LOGI("DS1338 RTC", "I2C device initialized ...");

  if (!rtc_config(&rtc)) {
    // config wasn't properly set. reboot.
    ESP_LOGW("DS1338 RTC", "clock source has been reconfigured. restarting!");
    fflush(stdout);
    esp_restart();
  }

  // time_t now;
  struct tm time;
  char timebuf[32];

  // set time once
  // time.tm_year = 2020 - 1900;
  // time.tm_mon = 8 - 1;
  // time.tm_mday = 25;
  // time.tm_hour = 22;
  // time.tm_min = 11;
  // time.tm_sec = 0;
  // ds1307_set_time(&rtc, &time);
  // vfd_text(vfd, "   OK");
  // while (true) {}


  while (true) {

    // ERROR: hitting "(xQueueGenericReceive)- assert failed!" here
    // when trying to take the mutex
    if (ds1307_get_time(&rtc, &time) != ESP_OK) {
      printf("couldn't get time from rtc");
    } else {
      // strftime(timebuf, sizeof(timebuf), "%H:%M", &time);
      snprintf(timebuf, sizeof(timebuf), "%02d%c%02d",
        time.tm_hour, (time.tm_sec % 2) == 1 ? ':' : ' ', time.tm_min);
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

  vfd_handle_t *vfd = chronovfd_init();
  
  // Pin 4, SENSOR_VP == GPIO 36 == ADC1 Channel 0
  ambientlight_dimmer_init(ADC1_CHANNEL_0, vfd->pin.fil_shdn);

  xTaskCreate(rtctime, "rtctime", 4096, vfd, 2, NULL);
  

}
