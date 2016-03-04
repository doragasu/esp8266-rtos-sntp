/*
 * Auxiliar functions to handle date/time along with lwIP sntp implementation.
 *
 * Jesus Alonso (doragasu)
 */

#include <stdio.h>
#include <espressif/esp_common.h>
#include <esp/timer.h>
#include <esp/rtc_regs.h>
#include "sntp.h"

#define TIMER_COUNT			RTC.COUNTER

// TODO: cal uses only 16 bits, use remaining 16 bits to store timezone and
// daylight settings
// Base calculated with value obtained from NTP server (64 bits)
#define sntp_base	(*((uint64_t*)RTC.SCRATCH))
// Timer value when base was obtained
#define tim_ref 	(RTC.SCRATCH[2])
// RTC ticks per microsecond, fixed point (Q20.12)
#define cal 		(RTC.SCRATCH[3])

// Timezone (from -11 to +13)
static int tz;
// Daylight (true if daylight savings enabled)
static bool dl;

void sntp_init(void);

// Sets time zone. Allowed values are in the range [-11, 13].
// NOTE: Settings do not take effect until SNTP time is updated. It is
void sntp_set_timezone(int time_zone) {
	tz = time_zone;
}

// Sets daylight.
// NOTE: Settings do not take effect until SNTP time is updated.
void sntp_set_daylight(int day_light) {
	dl = day_light;
}

void sntp_initialize(int time_zone, int day_light) {
	sntp_base = 0;
	tz = time_zone;
	dl = day_light;
	// To avoid div by 0 exceptions if requesting timebefore SNTP config
	cal = 1;
	tim_ref = TIMER_COUNT;
	sntp_init();
}

// Check if a timer wrap has occurred. Compensate sntp_base reference
// if affirmative.
// TODO: think about multitasking and race conditions
static inline void sntp_check_timer_wrap(uint32_t current_value) {
	if (current_value < tim_ref) {
		// Timer wrap has occurred, compensate by subtracting 2^32 to ref.
		sntp_base -= 1LLU<<32;
		// DEBUG
		printf("\nTIMER WRAPPED!\n");
	}
}

// Return secs. If us is not a null pointer, fill it with usecs
time_t sntp_get_rtc_time(int32_t *us) {
	time_t secs;
	uint32_t tim;
	uint64_t base;

	tim = TIMER_COUNT;
	// Check for timer wrap
	sntp_check_timer_wrap(tim);
	base = sntp_base + tim - tim_ref;
	secs = base * cal / (1000000U<<12);
//	printf("base=0x%X%08X, tref=0x%X, tim=0x%X, cal=0x%X\n", (uint32_t)(sntp_base>>32), (uint32_t)sntp_base,
//			tim_ref, tim, cal);
	if (us) {
		*us = base * cal % (1000000U<<12);
	}
	return secs;
}

/// Update RTC timer. Called by SNTP module each time it receives an update.
void sntp_update_rtc(time_t t, uint32_t us) {
	// Apply daylight and timezone correction
	t += (tz + dl) * 3600;
	// DEBUG: Compute and print drift
	int64_t sntp_current = sntp_base + TIMER_COUNT - tim_ref;
	int64_t sntp_correct = (((uint64_t)us + (uint64_t)t * 1000000U)<<12) / cal;
	printf("\nRTC Adjust: drift = %ld ticks, cal = %d\n", (time_t)(sntp_correct - sntp_current), cal);

	tim_ref = TIMER_COUNT;
	cal = sdk_system_rtc_clock_cali_proc();

	sntp_base = (((uint64_t)us + (uint64_t)t * 1000000U)<<12) / cal;

	// DEBUG: Print obtained secs and check calculated secs are the same
	time_t deb = sntp_base * cal / (1000000U<<12);
	printf("\nT: %lu, %lu, %s\n", t, deb, ctime(&deb));
	// DEBUG: Sleep 6 seconds and check time got increased as expected
}

