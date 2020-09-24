// Copyright (c) 2020 Anton Semjonov
// Licensed under the MIT License

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

#define SECONDS configTICK_RATE_HZ

// turn on the usr led, yay
#define LED 5
void led_init() {
  
  gpio_set_level(LED, 1);
}

/*
  TODO LIST
  - erase nvs and reprovision on long gpio0 button press
  - manage "foreground" task in stack with push/pop
  - use the temperature sensor
*/

void do_sync(TaskHandle_t clock) {

  esp_err_t err;
  if (clock) vTaskSuspend(clock);
  TaskHandle_t loading;
  animation_spinner(&loading);

  wireless_connect();
  err = xEventGroupWaitBits(wireless, WIFI_CONNECTED, false, true, 30 * SECONDS);
  if (err == ESP_ERR_TIMEOUT) goto fail;
  err = sntp_sync(60 * SECONDS);
  if (err == ESP_ERR_TIMEOUT) goto fail;
  wireless_disconnect();
  goto out;

  fail:
  vTaskSuspend(loading);
  vfd_text("FAIL");
  vTaskDelay(2 * SECONDS);

  out:
  vTaskDelete(loading);
  if (clock) vTaskResume(clock);

}

TaskHandle_t factory_reset;

void factory_reset_task(void *arg) {

  ESP_LOGW("factory_reset", "button pressed");
  TickType_t t = xTaskGetTickCount();
  for (int i = 0; i < 5; i++) {
    ESP_LOGW("factory_reset", "in %d ...", 5 - i);
    gpio_set_level(LED, 1);
    vTaskDelayUntil(&t, 0.2 * SECONDS);
    gpio_set_level(LED, 0);
    vTaskDelayUntil(&t, 0.8 * SECONDS);
  }
  gpio_set_level(LED, 1);
  ESP_LOGW("factory_reset", "erase nvs partition & reboot");
  nvs_flash_erase();
  esp_restart();

}

static void IRAM_ATTR factory_reset_isr(void *arg) {

  int level = gpio_get_level(GPIO_NUM_0);
  if (!level) {
    // falling edge, button press
    xTaskCreate(factory_reset_task, "factory reset", 2048, NULL, ESP_TASK_PRIO_MIN + 1, &factory_reset);
  } else {
    // rising edge, release
    if (factory_reset) vTaskDelete(factory_reset);
    gpio_set_level(LED, 0);
  }

}

void factory_reset_watcher_init() {

  gpio_config_t factrst = {
    .mode = GPIO_MODE_INPUT,
    .pin_bit_mask = (1ULL << GPIO_NUM_0),
    .pull_up_en = true,
    .intr_type = GPIO_INTR_ANYEDGE,
  };
  gpio_config(&factrst);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(GPIO_NUM_0, factory_reset_isr, NULL);

}

void app_main() {

  // initialize nonvolatile storage
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // init usr led
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_level(LED, 0);
  factory_reset_watcher_init();

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

  // connect to battery-backed rtc
  realtimeclock_init(I2C_SDA, I2C_SCL);
  time_t lastsync = realtimeclock_read_from_rtc();

  // set timezone to Europe/Berlin
  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  // enable and possibly provision credentials the first time
  // nvs_flash_erase(); // always clear creds to force conf
  wireless_init();
  wireless_provision();

  // maybe sync once on startup
  time_t now;
  time(&now);
  if ((lastsync == 0) || (lastsync > now) || ((now - lastsync) > (60 * 60))) {
    ESP_LOGI("main", "last sync more than an hour ago");
    do_sync(NULL);
  }

  // display the current time
  TaskHandle_t clock;
  clockface(&clock);
  
  for (;;) {
    sntp_sync_schedule(NULL);
    do_sync(clock);
  }

  wireless_end();
  
}
