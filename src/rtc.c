#include "rtc.h"
#include "ports.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static int get_update_in_progress_flag() {
    port_byte_out(CMOS_ADDRESS, 0x0A);
    return (port_byte_in(CMOS_DATA) & 0x80);
}

static unsigned char get_rtc_register(int reg) {
    port_byte_out(CMOS_ADDRESS, reg);
    return port_byte_in(CMOS_DATA);
}

void rtc_get_time(rtc_time_t *time) {
    if (!time) return;

    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned int year;

    unsigned char last_second;
    unsigned char last_minute;
    unsigned char last_hour;
    unsigned char last_day;
    unsigned char last_month;
    unsigned int last_year;
    unsigned char registerB;

    // Wait for update-in-progress to clear
    while (get_update_in_progress_flag());

    second = get_rtc_register(0x00);
    minute = get_rtc_register(0x02);
    hour   = get_rtc_register(0x04);
    day    = get_rtc_register(0x07);
    month  = get_rtc_register(0x08);
    year   = get_rtc_register(0x09);

    // Read repeatedly until we get two identical consecutive reads
    // to handle the rollover hazard gracefully
    do {
        last_second = second;
        last_minute = minute;
        last_hour   = hour;
        last_day    = day;
        last_month  = month;
        last_year   = year;

        while (get_update_in_progress_flag());

        second = get_rtc_register(0x00);
        minute = get_rtc_register(0x02);
        hour   = get_rtc_register(0x04);
        day    = get_rtc_register(0x07);
        month  = get_rtc_register(0x08);
        year   = get_rtc_register(0x09);
    } while ((last_second != second) || (last_minute != minute) || (last_hour != hour) ||
             (last_day != day) || (last_month != month) || (last_year != year));

    registerB = get_rtc_register(0x0B);

    // Convert BCD to binary representation if Register B Bit 2 is clear (BCD mode)
    if (!(registerB & 0x04)) {
        second = (second & 0x0F) + ((second / 16) * 10);
        minute = (minute & 0x0F) + ((minute / 16) * 10);
        hour   = ((hour & 0x0F) + (((hour & 0x70) / 16) * 10)) | (hour & 0x80);
        day    = (day & 0x0F) + ((day / 16) * 10);
        month  = (month & 0x0F) + ((month / 16) * 10);
        year   = (year & 0x0F) + ((year / 16) * 10);
    }

    // Convert 12-hour clock format to 24-hour if Register B Bit 1 is clear
    if (!(registerB & 0x02)) {
        int pm = hour & 0x80;
        hour &= 0x7F;
        if (pm) {
            if (hour != 12) {
                hour += 12;
            }
        } else {
            if (hour == 12) {
                hour = 0;
            }
        }
    }

    // Most PC platforms return a 2-digit year. Convert to 4-digit.
    year += 2000;

    time->second = second;
    time->minute = minute;
    time->hour   = hour;
    time->day    = day;
    time->month  = month;
    time->year   = year;
}