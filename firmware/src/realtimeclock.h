#pragma once

#include <esp_err.h>
#include "i2cdev.h"

#define I2C_SDA 21
#define I2C_SCL 22
#define RTC_MODEL "Maxim DS1338"

// battery-backed ds13xx i2c rtc
void realtimeclock_init(gpio_num_t sda, gpio_num_t scl);
void realtimeclock_read_from_rtc();
void realtimeclock_update_rtc();
void realtimeclock_update_rtc_fixedtime(const char *timestamp);

// tasks
void clockface_task(void *arg);
void clockface(TaskHandle_t *task);

// internet time synchronization
const char* sntp_servers[2];
esp_err_t sntp_sync(TickType_t timeout);