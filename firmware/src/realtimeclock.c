#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_sntp.h>
#include <esp_log.h>

#include "ds1307.h"
#include "vfddriver.h"
#include "realtimeclock.h"

// -------------------- battery-backed real-time clock --------------------

#define RTC_TAG "realtimeclock"

static i2c_dev_t ds13xx;

// check if the rtc is configured as expected and return if everything was ok
esp_err_t realtimeclock_check_configuration(i2c_dev_t *rtc) {
  esp_err_t ret = ESP_OK;

  bool ok;
  ESP_ERROR_CHECK(ds1307_is_running(rtc, &ok));
  if (!ok) {
    ESP_LOGI(RTC_TAG, "was halted. starting ...");
    ESP_ERROR_CHECK(ds1307_start(rtc, true));
    ret = ESP_ERR_INVALID_STATE;
  }

  ds1307_squarewave_freq_t sqwf;
  ESP_ERROR_CHECK(ds1307_get_squarewave_freq(rtc, &sqwf));
  if (sqwf != DS1307_32768HZ) {
    ESP_LOGI(RTC_TAG, "wrong SQWE frequency: RS = %x. setting 32.768 kHz ...", sqwf);
    ESP_ERROR_CHECK(ds1307_set_squarewave_freq(rtc, DS1307_32768HZ));
    ret = ESP_ERR_INVALID_STATE;
  }

  ESP_ERROR_CHECK(ds1307_is_squarewave_enabled(rtc, &ok));
  if (!ok) {
    ESP_LOGI(RTC_TAG, "SQWE output disabled. enabling ...");
    ESP_ERROR_CHECK(ds1307_enable_squarewave(rtc, true));
    ret = ESP_ERR_INVALID_STATE;
  }

  return ret;

}

void realtimeclock_init(gpio_num_t sda, gpio_num_t scl) {

  // initialize i2c device
  memset(&ds13xx, 0, sizeof(i2c_dev_t));
  ESP_ERROR_CHECK(ds1307_init_desc(&ds13xx, I2C_NUM_0, sda, scl));
  ESP_LOGI(RTC_TAG, "%s initialized", RTC_MODEL);

  // check register configuration
  if (realtimeclock_check_configuration(&ds13xx) == ESP_ERR_INVALID_STATE) {
    ESP_LOGW(RTC_TAG, "clock source has been reconfigured. restarting!");
    fflush(stdout);
    esp_restart();
  }

}

// update system time from battery-backed rtc
time_t realtimeclock_read_from_rtc() {

  struct tm t;
  ESP_ERROR_CHECK(ds1307_get_time(&ds13xx, &t));
  ESP_LOGI(RTC_TAG, "reading datetime from rtc: %04d-%02d-%02d %02d:%02d:%02d UTC",
    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
  struct timeval tv = { mktime(&t), 0 };
  settimeofday(&tv, NULL);

  time_t lastsync = realtimeclock_get_lastsync();
  ESP_LOGI(RTC_TAG, "rtc was last synchronized on %s", ctime(&lastsync));
  return lastsync;

}

// update rtc time from system, e.g. after sntp_sync
void realtimeclock_update_rtc() {

  time_t now;
  struct tm t;
  time(&now);
  gmtime_r(&now, &t);

  ESP_LOGI(RTC_TAG, "setting datetime in rtc: %04d-%02d-%02d %02d:%02d:%02d UTC",
    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
  ESP_ERROR_CHECK(ds1307_set_time(&ds13xx, &t));
  realtimeclock_set_lastsync(now);

}

// update rtc to a fixed time .. very crude way before I had sntp set up.
// takes iso-like strings: "2020-09-04 19:43:00"
void realtimeclock_update_rtc_fixedtime(const char *timestamp) {

  // parse timestamp string
  struct tm t;
  int n = sscanf(timestamp, "%4d-%2d-%2d %2d:%2d:%2d",
    &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec);
  if (n != 6) ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
  
  // apply corrections for expected rtc format
  t.tm_year -= 1900;
  t.tm_mon -= 1;

  // set system time and update rtc
  ESP_LOGW(RTC_TAG, "setting fixed time %s", timestamp);
  struct timeval tv = { mktime(&t), 0 };
  settimeofday(&tv, NULL);
  realtimeclock_update_rtc();

}

// offsets in rtc sram for different values
// ds1338 --> max. 56 bytes before wrapping to datetime values
#define RTC_VAL_LASTSYNC 0

void realtimeclock_set_lastsync(time_t t) {
  ESP_ERROR_CHECK(ds1307_write_ram(&ds13xx, RTC_VAL_LASTSYNC, (void*)&t, sizeof(time_t)));
}

time_t realtimeclock_get_lastsync() {
  time_t t;
  ESP_ERROR_CHECK(ds1307_read_ram(&ds13xx, RTC_VAL_LASTSYNC, (void*)&t, sizeof(time_t)));
  return t;
}

// -------------------- display tasks --------------------

// display the current clock in HH:MM on the display.
// assumes that the system rtc is synchronized with the external
// battery-backed source, so posix time() is used.
void clockface_task(void *arg) {

  time_t now;
  struct tm t;
  char timebuf[6];
  TickType_t lastwake = xTaskGetTickCount();

  for (;;) {
    time(&now);
    localtime_r(&now, &t);
    snprintf(timebuf, sizeof(timebuf), "%02d%c%02d",
      t.tm_hour, (t.tm_sec % 2) == 0 ? ':' : ' ', t.tm_min);
    vfd_text(timebuf);
    vTaskDelayUntil(&lastwake, pdMS_TO_TICKS(100));
  }

}

void clockface(TaskHandle_t *task) {
  xTaskCreate(clockface_task, "clockface", 2048, NULL, 10, task);
}

// -------------------- sntp time synchronization --------------------

#define SNTP_TAG "sntp_sync"
const char* sntp_servers[2] = {
  "0.de.pool.ntp.org",
  "1.de.pool.ntp.org",
};

static SemaphoreHandle_t sntp_sync_done;
void sntp_sync_callback() {
  ESP_LOGI(SNTP_TAG, "successfully synchronized");
  xSemaphoreGive(sntp_sync_done);
}

// Synchronize the time with a list of SNTP servers. Assumes that WiFi is
// running and a connection is established. Returns ESP_ERR_TIMEOUT if waiting
// for synchronization timed out.
esp_err_t sntp_sync(TickType_t timeout) {
  esp_err_t err = ESP_FAIL;

  ESP_LOGI(SNTP_TAG, "initialize");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  for (int i = 0; i < (sizeof(sntp_servers)/sizeof(char*)); i++) {
    sntp_setservername(i, sntp_servers[i]);
    ESP_LOGI(SNTP_TAG, "added server: %s", sntp_servers[i]);
  }
  if (!sntp_sync_done)
    sntp_sync_done = xSemaphoreCreateBinary();
  sntp_set_time_sync_notification_cb(sntp_sync_callback);
  sntp_init();

  ESP_LOGI(SNTP_TAG, "waiting %.1fs for synchronization", (double)timeout / configTICK_RATE_HZ);
  if (xSemaphoreTake(sntp_sync_done, timeout)) {
    err = ESP_OK;
    realtimeclock_update_rtc();
  } else {
    err = ESP_ERR_TIMEOUT;
    ESP_LOGW(SNTP_TAG, "timeout! failed to synchronize time");
  };
  sntp_stop();
  return err;
  
}
