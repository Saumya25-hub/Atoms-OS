#ifndef KERNEL_RTC_H
#define KERNEL_RTC_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
    uint64_t unix_timestamp;
} RTCDateTime;

bool rtc_read_datetime(RTCDateTime* dt_out);
uint64_t rtc_get_utc_timestamp(void);

#endif // KERNEL_RTC_H
