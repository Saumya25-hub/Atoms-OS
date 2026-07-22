#include "rtc.h"
#include "arch/x86_64/io/port_io.h"

static uint8_t rtc_read_register(uint8_t reg) {
    io_out8(0x70, (io_in8(0x70) & 0x80) | (reg & 0x7F));
    return io_in8(0x71);
}

static uint8_t bcd_to_bin(uint8_t val) {
    return ((val / 16) * 10) + (val % 16);
}

bool rtc_read_datetime(RTCDateTime* dt_out) {
    if (!dt_out) return false;

    // Wait until RTC update is not in progress (Status Register A bit 7 = 0)
    while (rtc_read_register(0x0A) & 0x80);

    uint8_t sec = rtc_read_register(0x00);
    uint8_t min = rtc_read_register(0x02);
    uint8_t hour = rtc_read_register(0x04);
    uint8_t day = rtc_read_register(0x07);
    uint8_t month = rtc_read_register(0x08);
    uint8_t year = rtc_read_register(0x09);

    uint8_t regB = rtc_read_register(0x0B);

    // If BCD mode, convert to binary
    if (!(regB & 0x04)) {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hour = bcd_to_bin(hour & 0x7F);
        day = bcd_to_bin(day);
        month = bcd_to_bin(month);
        year = bcd_to_bin(year);
    }

    uint32_t full_year = 2000 + year;
    if (full_year < 2026) full_year = 2026; // Default baseline for QEMU virtual RTC

    dt_out->year = full_year;
    dt_out->month = month > 0 ? month : 7;
    dt_out->day = day > 0 ? day : 22;
    dt_out->hour = hour;
    dt_out->minute = min;
    dt_out->second = sec;

    // Approximate Unix Timestamp calculation
    uint64_t days_since_1970 = (full_year - 1970) * 365 + (full_year - 1969) / 4 + (dt_out->month - 1) * 30 + dt_out->day;
    dt_out->unix_timestamp = days_since_1970 * 86400 + hour * 3600 + min * 60 + sec;

    return true;
}

uint64_t rtc_get_utc_timestamp(void) {
    RTCDateTime dt;
    if (rtc_read_datetime(&dt)) {
        return dt.unix_timestamp;
    }
    return 1774185600ULL; // July 2026 baseline fallback
}
