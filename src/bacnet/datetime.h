/**
 * @file
 * @brief API for BACnetDate, BACnetTime, BACnetDateTime, BACnetDateRange 
 * complex data type encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Greg Shue <greg.shue@outlook.com>
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @date 2012
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DATE_TIME_H
#define BACNET_DATE_TIME_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

// OS clock usage
// 
// Coupled 
//      Use the OS clock. Setting date and time via BACnet TimeSynch (etc) services modifies date and time of the underlying OS
// 
// Decoupled
//      Use a 'tracking' clock that references the OS clock but adds a 'ClockOffset', initially 0 [*1],
//      allowing free and easy manipulation of 'BACnet Time' without affecting the underlying OS clock.
// 
//      It also allows write access to the Daylight_Savings_Status and UTC_Offset properties, which would otherwise be controlled by the OS
// 
//          This makes testing of Calendars, Schedules and Trend Logs easier on 'sophisticated' platforms, 
//          e.g. linux, Windows, VMs etc.  It works just as well as 'Coupled' time when it comes to BACnet, but
//          other apps on running on the same platform can continue to use the OS time, from e.g. OS SNTP, GPS clocks etc.
//          You may want to persist this offset through system restarts.
// 
//      Much discussion:
// 
//          In general, there are 3 modes of possible Time and Date operation on a BACnet Device
//          
//          1   OS controls BACnet time. Modifying BACnet time modifies OS time.
// 
//          2   UTC_Offset and Daylight_Savings_Status is completely controlled BACnet 'over the wire' messages. For very simple devices
// 
//          3   A hybrid, which accomodates testing, (this 'DECOUPLED_BACNET_TIME') implementation.
// 
//              -   On restart, the BACnet application reads the OS time, Offset and DST and applies them to the BACnet properties.
//                  (And [*1] load persistent ClockOffset if available.)
//              -   BACnet time that is presented and used is the OS time + ClockOffset
//              -   Modifying UTC and Local date and time behaves as expected, but only ClockOffset is modified.
//              -   DST is maintained by the OS, until.. 
//              -   If BACnet UTC_Offset is written, if allowed, this takes precedence over OS value until either a restart or Local settings are touched [*2]
//              -   Writing to BACnet Daylight_Savings_Status, if allowed, overwrites the OS setting, and this remains overridden until 
//                  either restart or Local settings are touched [*2]
//              -   UTCtimesych updates both UTC (internal) and Local_Time and Local_Date as expected (by modifying ClockOffset, not OS)
//              -   Write to Local_Time or Local_Date updates the ClockOffset as expected
//              -   Similarly for BACnet UTC_Offset
// 
// Notes:
// 
//  1   Ideally, this value should be persisted
//  2   'Touching Local settings': if TimeSynchronize (local) sent, or write to Local_Time or Local_Date settings occur, or system restarts (warm, cold or power)
//



#define USE_DECOUPLED_BACNET_TIME   1


/* define our epic beginnings */
#define BACNET_DATE_YEAR_EPOCH 1900
/* 1/1/1900 is a Monday */
#define BACNET_DAY_OF_WEEK_EPOCH BACNET_WEEKDAY_MONDAY

typedef enum BACnet_Weekday {
    BACNET_WEEKDAY_MONDAY = 1,
    BACNET_WEEKDAY_TUESDAY = 2,
    BACNET_WEEKDAY_WEDNESDAY = 3,
    BACNET_WEEKDAY_THURSDAY = 4,
    BACNET_WEEKDAY_FRIDAY = 5,
    BACNET_WEEKDAY_SATURDAY = 6,
    BACNET_WEEKDAY_SUNDAY = 7,
    BACNET_WEEKDAY_ANY = 0xff
} BACNET_WEEKDAY;

/* date */
typedef struct BACnet_Date {
    uint16_t year; /* AD */
    uint8_t month; /* 1=Jan */
    uint8_t day; /* 1..31 */
    uint8_t wday; /* 1=Monday-7=Sunday */
} BACNET_DATE;

/* time */
typedef struct BACnet_Time {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t hundredths;
} BACNET_TIME;

typedef struct BACnet_DateTime {
    BACNET_DATE date;
    BACNET_TIME time;
} BACNET_DATE_TIME;

/* range of dates */
typedef struct BACnet_Date_Range {
    BACNET_DATE startdate;
    BACNET_DATE enddate;
} BACNET_DATE_RANGE;

/* week and days */
typedef struct BACnet_Weeknday {
    uint8_t month; /* 1=Jan 13=odd 14=even FF=any */
    uint8_t weekofmonth; /* 1=days 1-7, 2=days 8-14, 3=days 15-21, 4=days 22-28,
                            5=days 29-31, 6=last 7 days, FF=any week */
    uint8_t dayofweek; /* 1=Monday-7=Sunday, FF=any */
} BACNET_WEEKNDAY;

#ifdef UINT64_MAX
typedef uint64_t bacnet_time_t;
#else
typedef uint32_t bacnet_time_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility initialization functions */
BACNET_STACK_EXPORT
void datetime_set_date(
    BACNET_DATE *bdate, uint16_t year, uint8_t month, uint8_t day);
BACNET_STACK_EXPORT
void datetime_set_time(BACNET_TIME *btime,
    uint8_t hour,
    uint8_t minute,
    uint8_t seconds,
    uint8_t hundredths);
BACNET_STACK_EXPORT
void datetime_set(
    BACNET_DATE_TIME *bdatetime, BACNET_DATE *bdate, BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_set_values(BACNET_DATE_TIME *bdatetime,
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t hour,
    uint8_t minute,
    uint8_t seconds,
    uint8_t hundredths);
BACNET_STACK_EXPORT
uint32_t datetime_hms_to_seconds_since_midnight(
    uint8_t hours, uint8_t minutes, uint8_t seconds);
BACNET_STACK_EXPORT
uint16_t datetime_hm_to_minutes_since_midnight(uint8_t hours, uint8_t minutes);
BACNET_STACK_EXPORT
void datetime_hms_from_seconds_since_midnight(
    uint32_t seconds, uint8_t *pHours, uint8_t *pMinutes, uint8_t *pSeconds);
/* utility test for validity */
BACNET_STACK_EXPORT
bool datetime_is_valid(BACNET_DATE *bdate, BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_time_is_valid(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_date_is_valid(BACNET_DATE *bdate);
/* date and time calculations and summaries */
BACNET_STACK_EXPORT
void datetime_ymd_from_days_since_epoch(
    uint32_t days, uint16_t *pYear, uint8_t *pMonth, uint8_t *pDay);
BACNET_STACK_EXPORT
uint32_t datetime_days_since_epoch(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_days_since_epoch_into_date(uint32_t days, BACNET_DATE *bdate);
BACNET_STACK_EXPORT
uint32_t datetime_day_of_year(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
uint32_t datetime_ymd_to_days_since_epoch(
    uint16_t year, uint8_t month, uint8_t day);
BACNET_STACK_EXPORT
void datetime_day_of_year_into_date(
    uint32_t days, uint16_t year, BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_is_leap_year(uint16_t year);
BACNET_STACK_EXPORT
uint8_t datetime_month_days(uint16_t year, uint8_t month);
BACNET_STACK_EXPORT
uint8_t datetime_day_of_week(uint16_t year, uint8_t month, uint8_t day);
BACNET_STACK_EXPORT
bool datetime_ymd_is_valid(uint16_t year, uint8_t month, uint8_t day);
BACNET_STACK_EXPORT
uint32_t datetime_ymd_day_of_year(uint16_t year, uint8_t month, uint8_t day);

BACNET_STACK_EXPORT
void datetime_seconds_since_midnight_into_time(
    uint32_t seconds, BACNET_TIME *btime);
BACNET_STACK_EXPORT
uint32_t datetime_seconds_since_midnight(BACNET_TIME *btime);
BACNET_STACK_EXPORT
uint16_t datetime_minutes_since_midnight(BACNET_TIME *btime);

/* utility comparison functions:
   if the date/times are the same, return is 0
   if date1 is before date2, returns negative
   if date1 is after date2, returns positive */
BACNET_STACK_EXPORT
int datetime_compare_date(BACNET_DATE *date1, BACNET_DATE *date2);
BACNET_STACK_EXPORT
int datetime_compare_time(BACNET_TIME *time1, BACNET_TIME *time2);
BACNET_STACK_EXPORT
int datetime_compare(BACNET_DATE_TIME *datetime1, BACNET_DATE_TIME *datetime2);

/* full comparison functions:
 * taking into account FF fields in date and time structures,
 * do a full comparison of two values */
BACNET_STACK_EXPORT
int datetime_wildcard_compare_date(BACNET_DATE *date1, BACNET_DATE *date2);
BACNET_STACK_EXPORT
int datetime_wildcard_compare_time(BACNET_TIME *time1, BACNET_TIME *time2);
BACNET_STACK_EXPORT
int datetime_wildcard_compare(
    BACNET_DATE_TIME *datetime1, BACNET_DATE_TIME *datetime2);

/* utility copy functions */
BACNET_STACK_EXPORT
void datetime_copy_date(BACNET_DATE *dest, BACNET_DATE *src);
BACNET_STACK_EXPORT
void datetime_copy_time(BACNET_TIME *dest, BACNET_TIME *src);
BACNET_STACK_EXPORT
void datetime_copy(BACNET_DATE_TIME *dest, BACNET_DATE_TIME *src);

/* utility add or subtract minutes function */
BACNET_STACK_EXPORT
void datetime_add_minutes(BACNET_DATE_TIME *bdatetime, int32_t minutes);

BACNET_STACK_EXPORT
bacnet_time_t datetime_seconds_since_epoch(BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
void datetime_since_epoch_seconds(
    BACNET_DATE_TIME *bdatetime, bacnet_time_t seconds);
BACNET_STACK_EXPORT
bacnet_time_t datetime_seconds_since_epoch_max(void);

/* date and time wildcards */
BACNET_STACK_EXPORT
bool datetime_wildcard_year(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_year_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_month(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_month_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_day(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_day_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_weekday(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_weekday_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_hour(BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_hour_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard_minute(BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_minute_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard_second(BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_second_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard_hundredths(BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_hundredths_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard(BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
bool datetime_wildcard_present(BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
void datetime_wildcard_set(BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
void datetime_date_wildcard_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_time_wildcard_set(BACNET_TIME *btime);

BACNET_STACK_EXPORT
bool datetime_local_to_utc(BACNET_DATE_TIME *utc_time,
    BACNET_DATE_TIME *local_time,
    int16_t utc_offset_minutes,
    int8_t dst_adjust_minutes);
BACNET_STACK_EXPORT

bool datetime_utc_to_local(BACNET_DATE_TIME *local_time,
    BACNET_DATE_TIME *utc_time,
    int16_t utc_offset_minutes,
    int8_t dst_adjust_minutes);

BACNET_STACK_EXPORT
bool datetime_date_init_ascii(BACNET_DATE *bdate, const char *ascii);
BACNET_STACK_EXPORT
int datetime_date_to_ascii(BACNET_DATE *bdate, char *str, size_t str_size);
BACNET_STACK_EXPORT
bool datetime_time_init_ascii(BACNET_TIME *btime, const char *ascii);
BACNET_STACK_EXPORT
int datetime_time_to_ascii(BACNET_TIME *btime, char *str, size_t str_size);
BACNET_STACK_EXPORT
bool datetime_init_ascii(BACNET_DATE_TIME *bdatetime, const char *ascii);
BACNET_STACK_EXPORT
int datetime_to_ascii(BACNET_DATE_TIME *bdatetime, char *str, size_t str_size);

BACNET_STACK_EXPORT
int bacapp_encode_datetime(uint8_t *apdu, BACNET_DATE_TIME *value);
BACNET_STACK_EXPORT
int bacapp_encode_context_datetime(
    uint8_t *apdu, uint8_t tag_number, BACNET_DATE_TIME *value);
BACNET_STACK_EXPORT
int bacnet_datetime_decode(
    uint8_t *apdu, uint32_t apdu_size, BACNET_DATE_TIME *value);
BACNET_STACK_EXPORT
int bacnet_datetime_context_decode(
    uint8_t *apdu, uint32_t apdu_size,  uint8_t tag_number,
    BACNET_DATE_TIME *value);

BACNET_STACK_DEPRECATED("Use bacnet_datetime_decode() instead")
BACNET_STACK_EXPORT
int bacapp_decode_datetime(
    uint8_t *apdu, BACNET_DATE_TIME *value);
BACNET_STACK_DEPRECATED("Use bacnet_datetime_context_decode() instead")
BACNET_STACK_EXPORT
int bacapp_decode_context_datetime(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_DATE_TIME *value);

BACNET_STACK_EXPORT
int bacnet_daterange_encode(uint8_t *apdu, BACNET_DATE_RANGE *value);
BACNET_STACK_EXPORT
int bacnet_daterange_decode(uint8_t *apdu, 
    uint32_t apdu_size, 
    BACNET_DATE_RANGE *value);
BACNET_STACK_EXPORT
int bacnet_daterange_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_DATE_RANGE *value);
BACNET_STACK_EXPORT
int bacnet_daterange_context_decode(uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_DATE_RANGE *value);

/* implementation agnostic functions - create your own! */
BACNET_STACK_EXPORT
bool datetime_local(BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active);
BACNET_STACK_EXPORT
void datetime_init(void);

BACNET_STACK_EXPORT
bool datetime_utc_set(BACNET_DATE_TIME* bdt);
BACNET_STACK_EXPORT
bool datetime_local_set(BACNET_DATE_TIME* bdt);
BACNET_STACK_EXPORT
bool datetime_UTC_Offset_set(int offset, BACNET_ERROR_CODE* wp_errCode);
BACNET_STACK_EXPORT
int datetime_UTC_Offset_get(void);
BACNET_STACK_EXPORT
bool datetime_DST_set(bool dstFlag, BACNET_ERROR_CODE* wp_errCode);
BACNET_STACK_EXPORT
bool datetime_DST_get(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
