#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

// event group for status bits: init, provisioned, associated, connected
EventGroupHandle_t wireless;
#define WIFI_INITIALIZED   1 << 0
#define WIFI_PROVISIONED   1 << 1
#define WIFI_ASSOCIATED    1 << 2
#define WIFI_CONNECTED     1 << 3
#define WIFI_WANTED        1 << 4

void wireless_init();
void wireless_end();
void wireless_provision();
void wireless_connect();
void wireless_disconnect();
