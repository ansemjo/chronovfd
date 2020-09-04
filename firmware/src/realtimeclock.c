#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "ds1307.h"
#include "vfddriver.h"
#include "realtimeclock.h"


i2c_dev_t chrono_rtc_init() {

  i2c_dev_t rtc;
  memset(&rtc, 0, sizeof(i2c_dev_t));

  // initialize i2c device
  ESP_ERROR_CHECK(ds1307_init_desc(&rtc, I2C_NUM_0, 21, 22));
  ESP_LOGI(RTC_TAG, "I2C device initialized.");

  if (!chrono_rtc_config(&rtc)) {
    // config wasn't properly set. reboot.
    ESP_LOGW(RTC_TAG, "clock source has been reconfigured. restarting!");
    fflush(stdout);
    esp_restart();
  }

  return rtc;

}

// check if the rtc is configured as expected and return if everything was ok
bool chrono_rtc_config(i2c_dev_t *rtc) {

  bool ret = true;

  bool ok;
  ESP_ERROR_CHECK(ds1307_is_running(rtc, &ok));
  if (!ok) {
    ESP_LOGI(RTC_TAG, "wasn't started. starting ...");
    ESP_ERROR_CHECK(ds1307_start(rtc, true));
    ret = false;
  }

  ds1307_squarewave_freq_t sqwf;
  ESP_ERROR_CHECK(ds1307_get_squarewave_freq(rtc, &sqwf));
  if (sqwf != DS1307_32768HZ) {
    ESP_LOGI(RTC_TAG, "wrong SQWE frequency: RS = %x. setting 32.768 kHz ...", sqwf);
    ESP_ERROR_CHECK(ds1307_set_squarewave_freq(rtc, DS1307_32768HZ));
    ret = false;
  }

  ESP_ERROR_CHECK(ds1307_is_squarewave_enabled(rtc, &ok));
  if (!ok) {
    ESP_LOGI(RTC_TAG, "SQWE output not enabled. enabling ...");
    ESP_ERROR_CHECK(ds1307_enable_squarewave(rtc, true));
    ret = false;
  }

  return ret;

}

// update system time from battery-backed rtc
void update_time_from_rtc(i2c_dev_t *rtc) {

  struct tm update;
  struct timeval tv;
  ESP_ERROR_CHECK(ds1307_get_time(rtc, &update));
  ESP_LOGI(RTC_TAG, "Reading Datetime [UTC]: %04d-%02d-%02d %02d:%02d:%02d",
    update.tm_year + 1900, update.tm_mon + 1, update.tm_mday,
    update.tm_hour, update.tm_min, update.tm_sec);
  fflush(stdout);
  
  tv.tv_sec = mktime(&update);
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);

}

void update_time_in_rtc(i2c_dev_t *rtc) {

  time_t now;
  struct tm update;
  time(&now);
  gmtime_r(&now, &update);

  ESP_LOGI(RTC_TAG, "Setting Datetime [UTC]: %04d-%02d-%02d %02d:%02d:%02d",
    update.tm_year + 1900, update.tm_mon + 1, update.tm_mday,
    update.tm_hour, update.tm_min, update.tm_sec);
  ESP_ERROR_CHECK(ds1307_set_time(rtc, &update));

}

// display the current clock in HH:MM on the display.
// assumes that the esp-internal rtc is synchronized with the external
// battery-backed source, so time() is used.
void timedisplay_task(void *arg) {

  // cast argument to vfd display handle
  vfd_handle_t *vfd = arg;

  time_t now;
  struct tm tm;
  char timebuf[32];

  while (true) {
    time(&now);
    localtime_r(&now, &tm);
    snprintf(timebuf, sizeof(timebuf), "%02d%c%02d",
      tm.tm_hour, (tm.tm_sec % 2) == 1 ? ':' : ' ', tm.tm_min);
    vfd_text(vfd, timebuf);
    vTaskDelay(100 / portTICK_RATE_MS);
  }
  
  // shouldn't ever get here
  vTaskDelete(NULL);
}

// set time once .. very crude way to set time before i have sntp set up
void chrono_rtc_set_fixed_time(i2c_dev_t *rtc, int year, int month, int day,
  int hour, int minute, int second) {

  struct tm nt;
  nt.tm_year = year - 1900;
  nt.tm_mon = month - 1;
  nt.tm_mday = day;
  nt.tm_hour = hour;
  nt.tm_min = minute;
  nt.tm_sec = second;
  ds1307_set_time(rtc, &nt);
  time_t now = mktime(&nt);
  printf("+++ set time to fixed date: %s +++\n", ctime(&now));
  while (true) {
    printf(".");
    vTaskDelay(1000 / portTICK_RATE_MS);
  }
}