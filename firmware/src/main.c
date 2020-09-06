#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_types.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_wifi.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include <driver/gpio.h>

#include "ds1307.h"
#include "vfddriver.h"
#include "segments.h"
#include "realtimeclock.h"
#include "ambientlight.h"
#include "animations.h"
#include "wireless.h"

// turn on the usr led, yay
#define LED 5
void led_init() {
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_level(LED, 1);
}

void app_main() {

  // initialize nonvolatile storage
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // initialize vacuum flourescent display
  vfd_pin_t vfdcfg = {
    .enable    = VFD_PIN_ENABLE,
    .clock     = VFD_PIN_CLOCK,
    .data      = VFD_PIN_DATA,
    .strobe    = VFD_PIN_STROBE,
    .blank     = VFD_PIN_BLANK,
    .fil_shdn  = VFD_PIN_FILSHDN,
    .hv_shdn   = VFD_PIN_HVSHDN,
  };
  vfd_init_spi(&vfdcfg);
  TaskHandle_t digitmux;
  vfd_init_mux(0.002, &digitmux);

  // initialize filament dimmer via photodiode
  TaskHandle_t ambientlight;
  ambientlight_init(PHOTODIODE, vfdcfg.fil_shdn, &ambientlight);

  // TODO: new provisioning attempt
  nvs_flash_erase(); // always clear creds to force conf
  wireless_init();
  wireless_begin();
  wireless_provision();

  // connect to battery-backed rtc
  realtimeclock_init(I2C_SDA, I2C_SCL);
  realtimeclock_read_from_rtc();

  // set timezone to Europe/Berlin
  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  // display the current time
  TaskHandle_t clock;
  clockface(&clock);
  
  vTaskDelay(3000/portTICK_RATE_MS);
  
  // enable wifi and synchronize time via ntp  
  vTaskSuspend(clock);
  TaskHandle_t loading;
  animation_spinner(&loading);
  
  sntp_sync(pdMS_TO_TICKS(20000));
  wireless_end();
  
  vTaskDelete(loading);
  vTaskResume(clock);
  
}
