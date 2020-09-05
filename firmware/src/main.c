#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_types.h>
#include <esp_wifi.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_sntp.h>

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <driver/periph_ctrl.h>
#include <driver/timer.h>
#include <driver/ledc.h>
#include <driver/adc.h>

#include "ds1307.h"

#include "app_prov.h"
#include "vfddriver.h"
#include "segments.h"
#include "realtimeclock.h"
#include "ambientlight.h"
#include "animations.h"

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

void wifi_provision() {

  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  bool provisioned = false;
  ESP_ERROR_CHECK(app_prov_is_provisioned(&provisioned));
  if (!provisioned) {

    TaskHandle_t loading;
    animation_textfader(&loading, "Conf");

    ESP_LOGI("chronovfd", "begin wifi provisioning");
    // https://github.com/espressif/esp-idf/blob/v4.0.1/examples/provisioning/ble_prov/main/app_main.c
    
    wifi_prov_security_t sec = WIFI_PROV_SECURITY_0;
    const protocomm_security_pop_t *pop = NULL;
    ESP_ERROR_CHECK(app_prov_start_ble_provisioning(sec, pop));
    
    // once provisioning is started wait until its data struct is freed
    while (g_prov != NULL) {
      vTaskDelay(pdMS_TO_TICKS(500));
    }

    vTaskDelete(loading);
  
  };

  esp_wifi_stop();
  esp_wifi_deinit();
  esp_event_loop_delete_default();


}

void wifi_init() {

  static char* tag = "chronowifi";

  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_LOGI(tag, "starting station");
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

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

  wifi_provision();

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
  
  ESP_LOGI("main", "enable wifi");
  wifi_init();
  
  sntp_sync(pdMS_TO_TICKS(20000));
  
  ESP_LOGI("main", "stop wifi");
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_event_loop_delete_default();
  
  vTaskDelete(loading);
  vTaskResume(clock);
  
}
