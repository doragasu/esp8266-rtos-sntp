#ifndef __SNTP_H__
#define __SNTP_H__

#include <stdint.h>
#include <time.h>

/// Update SNTP RTC timer
void sntp_update_rtc(time_t t, uint32_t us);

/// Uncomment to get time in microseconds
#define SNTP_SET_SYSTEM_TIME_US(sec, us) sntp_update_rtc(sec, us)

// SNTP update delay in milliseconds (minimum allowed value is 15 seconds)
#define SNTP_UPDATE_DELAY           (5*60000)

/// Time without usec resolution. Not used if _US version defined.
#define SNTP_SET_SYSTEM_TIME(sec) sntp_update_rtc(sec, 0)

/// For the lwIP implementation of SNTP to allow using names for NTP servers.
#define SNTP_SERVER_DNS             1

void sntp_initialize(int time_zone, int day_light);

// Sets time zone. Allowed values are in the range [-11, 13].
// NOTE: Settings do not take effect until SNTP time is updated. It is
void sntp_set_timezone(int time_zone);

// Sets daylight.
// NOTE: Settings do not take effect until SNTP time is updated.
void sntp_set_daylight(int day_light);

time_t sntp_get_rtc_time(int32_t *us);

#endif /* __SNTP_H__ */
