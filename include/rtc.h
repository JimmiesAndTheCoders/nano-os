#ifndef RTC_H
#define RTC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned int year;
} rtc_time_t;

void rtc_get_time(rtc_time_t *time);

#ifdef __cplusplus
}
#endif

#endif