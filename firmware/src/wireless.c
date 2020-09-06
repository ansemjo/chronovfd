// #include <stdlib.h>
// #include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_types.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_wifi.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include "animations.h"
#include "wireless.h"

// tag for logging
static const char *tag = "wireless";

/* Signal Wi-Fi events on this event-group */
const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;

// return a service name for provisioning
static void get_device_name(char *name, size_t max) {
    const char *prefix = "PROV_CHRONO_";
    uint8_t mac[6];
    if (max < (sizeof(prefix) + 6)) { ESP_ERROR_CHECK(ESP_ERR_INVALID_SIZE); }
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
    snprintf(name, max, "%s%02X%02X%02X", prefix, mac[3], mac[4], mac[5]);
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  static const char *tag = "wireless_event_handler";
  if (event_base == WIFI_EVENT) {
    static char* tag = "wifi";
    switch (event_id) {

      case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(tag, "wifi disconnected. trying to connect again ...");
        __attribute__((fallthrough));
      case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;

      default:
        break;

    }
  } else if (event_base == IP_EVENT) {
    switch (event_id) {

      case IP_EVENT_STA_GOT_IP: {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(tag, "received IP: %s", ip4addr_ntoa(&event->ip_info.ip));
        break;
      }

      default:
        break;

    }
  } else if (event_base == WIFI_PROV_EVENT) {
    switch (event_id) {

      case WIFI_PROV_START:
        ESP_LOGI(tag, "provisioning started");
        break;

      case WIFI_PROV_CRED_RECV: {
        wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
        ESP_LOGI(tag, "received credentials for SSID %s", (const char *) wifi_sta_cfg->ssid);
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
        break;
      }

      case WIFI_PROV_CRED_SUCCESS:
        ESP_LOGI(tag, "provisioning success");
        break;

      case WIFI_PROV_END:
        ESP_LOGI(tag, "provisioning finished, deinit manager ..");
        wifi_prov_mgr_deinit();
        break;

      default:
        break;
    
    }
  }
}

void wireless_init() {

  ESP_LOGI(tag, "initialize TCP/IP stack, create event group");
  tcpip_adapter_init();
  wifi_event_group = xEventGroupCreate();
  if (wifi_event_group == NULL)
    ESP_ERROR_CHECK(ESP_ERR_NO_MEM);

}

void wireless_begin() {

  ESP_LOGI(tag, "start wifi stack");
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  
  ESP_LOGI(tag, "register event handlers");
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
  
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wireless_provision();
  ESP_ERROR_CHECK(esp_wifi_start());

}

void wireless_end() {

  ESP_LOGI(tag, "stop and deinit");
  ESP_ERROR_CHECK(esp_wifi_stop());
  ESP_ERROR_CHECK(esp_wifi_deinit());
  ESP_ERROR_CHECK(esp_event_loop_delete_default());

}

void wireless_provision() {

  // see esp-idf examples for fully commented provisioning flow
  // https://github.com/espressif/esp-idf/blob/v4.0.1/examples/provisioning/manager/main/app_main.c#L152
  // https://docs.espressif.com/projects/esp-idf/en/v4.0.1/api-reference/provisioning/wifi_provisioning.html
  
  wifi_prov_mgr_config_t provcfg = {
    .scheme = wifi_prov_scheme_ble,
    .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
  };
  ESP_ERROR_CHECK(wifi_prov_mgr_init(provcfg));

  // check if already provisioned
  bool provisioned = false;
  ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
  if (provisioned) {
    ESP_LOGI(tag, "credentials already provisioned");
    goto deinit;
  }

  TaskHandle_t conffader;
  animation_textfader(&conffader, "Prov");
  
  char service_name[19];
  get_device_name(service_name, sizeof(service_name));

  // no encryption, no proof-of-posession, ble requires no key
  wifi_prov_security_t sec = WIFI_PROV_SECURITY_0;
  const char *pop = NULL;
  const char *key = NULL;

  // use the same service uuid from esp-idf example
  uint8_t example_service_uuid[] = {
    0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
    0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
  };
  ESP_ERROR_CHECK(wifi_prov_scheme_ble_set_service_uuid(example_service_uuid));

  // begin provisioning
  ESP_LOGI(tag, "start provisioning with BLE method");
  ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(sec, pop, service_name, key));

  // block until provisioning is done and connection is established
  wifi_prov_mgr_wait();
  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);
  vTaskDelete(conffader);
  ESP_LOGI(tag, "provisioning done");

  // deinit the provisioning manager at the end
  deinit: wifi_prov_mgr_deinit();

}
