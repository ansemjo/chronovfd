#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_types.h"
#include "esp_wifi.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_sntp.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "driver/ledc.h"
#include "driver/adc.h"

#include "ds1307.h"

#include "app_prov.h"
#include "vfddriver.h"
#include "segments.h"
#include "realtimeclock.h"
#include "ambientlight.h"

// turn on the usr led, yay
#define LED 5
void led_init() {
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_level(LED, 1);
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT) {
    // static char* tag = "wifi";
    switch (event_id) {

      case WIFI_EVENT_STA_START:
      case WIFI_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        break;

      default:
        break;

    }
  } else if (event_base == IP_EVENT) {
    static char* tag = "networking";
    switch (event_id) {

      case IP_EVENT_STA_GOT_IP: {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(tag, "got ip: %s", ip4addr_ntoa(&event->ip_info.ip));
        break;
      }

      default:
        break;

    }
  } else if (event_base == WIFI_PROV_EVENT) {
    static char* tag = "wifi provisioning";
    switch (event_id) {

      case WIFI_PROV_START:
        ESP_LOGI(tag, "started");
        break;

      case WIFI_PROV_CRED_RECV: {
        wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
        ESP_LOGI(tag, "Received credentials"
          "\n\tSSID     : %s\n\tPassword : %s",
          (const char *) wifi_sta_cfg->ssid,
          (const char *) wifi_sta_cfg->password);
        break;
      }

      case WIFI_PROV_CRED_SUCCESS:
        ESP_LOGI(tag, "success!");
        break;

      case WIFI_PROV_END:
        wifi_prov_mgr_deinit();
        break;

      default:
        break;
    
    }
  }
}

void wifi_init() {

  static char* tag = "chronowifi";

  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  bool provisioned = false;
  ESP_ERROR_CHECK(app_prov_is_provisioned(&provisioned));
  if (!provisioned) {

    ESP_LOGI(tag, "begin wifi provisioning");
    // https://github.com/espressif/esp-idf/blob/v4.0.1/examples/provisioning/ble_prov/main/app_main.c
    
    wifi_prov_security_t sec = WIFI_PROV_SECURITY_0;
    const protocomm_security_pop_t *pop = NULL;
    app_prov_start_ble_provisioning(sec, pop);
    // TODO: block here until provisioning is done
  
  } else {

    ESP_LOGI(tag, "starting station");
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

  }

}

void synctime_cb() {
  ESP_LOGI("sntp", "time synchronized!");
}

void synctime() {
  static const char* tag = "sntp";

  ESP_LOGI(tag, "initialize sntp");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "0.de.pool.ntp.org");
  sntp_set_time_sync_notification_cb(synctime_cb);
  sntp_init();

  int retry = 0;
  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < 100) {
    ESP_LOGI(tag, "waiting for sync ...");
    vTaskDelay(300/portTICK_RATE_MS);
    retry++;
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

  i2c_dev_t rtc = chrono_rtc_init();
  update_time_from_rtc(&rtc);

  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  TaskHandle_t clock;
  xTaskCreate(timedisplay_task, "clockface", 4096, vfd, 2, &clock);

  // try to configure wifi with provisioning and sync time
  // caution: display begins to flicker while wifi is active
  // TODO: needs more "state machine" work: when to sync? block during provisioning?
  // TODO: connect wifi after provisioning? etc. ... but works for me™ already

  vTaskDelay(10000/portTICK_RATE_MS);
  vTaskSuspend(clock);
  vfd_text(vfd, "sn tp");
  ESP_LOGI("chronovfd", "enable wifi");
  wifi_init();
  synctime();
  update_time_in_rtc(&rtc);
  ESP_LOGI("chronovfd", "done, stop wifi");
  esp_wifi_stop();
  vTaskResume(clock);
  
}
