#ifndef _REALTIMECLOCK_H_
#define _REALTIMECLOCK_H_

#include "ds1307.h"

// the tag used for esp logging
#define RTC_TAG "DS1338 RTC"

i2c_dev_t chrono_rtc_init();
bool chrono_rtc_config(i2c_dev_t *rtc);
void update_time_from_rtc(i2c_dev_t *rtc);
void update_time_in_rtc(i2c_dev_t *rtc);
void timedisplay_task(void *arg);

#endif // _REALTIMECLOCK_H_