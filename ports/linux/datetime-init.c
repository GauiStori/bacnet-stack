/**
 * @file
 * @author Steve Karg
 * @date 2009
 * @brief System time library header file.
 *
 * @section DESCRIPTION
 *
 * This library provides functions for getting and setting the system time.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "bacport.h"
#include "bacnet/datetime.h"
#include "bacnet/wp.h"

#ifndef USE_DECOUPLED_BACNET_TIME
#error - ensure this gets set in datetime.h
#endif 

#if ( USE_DECOUPLED_BACNET_TIME == 1 )

// todo - you may want to persist this value
static time_t decoupledTimeOffset;

static int16_t UTC_Offset;
static bool Daylight_Savings_Status;

#endif 


/**
 * @brief Get the date, time, timezone, and UTC offset from system
 * @param utc_time - the BACnet Date and Time structure to hold UTC time
 * @param local_time - the BACnet Date and Time structure to hold local time
 * @param utc_offset_minutes - number of minutes offset from UTC
 * For example, -6*60 represents 6.00 hours behind UTC/GMT
 * @param true if DST is enabled and active
 * @return true if local time was retrieved
 */
bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    bool status = false;
    struct tm *tblock = NULL;
    struct timeval tv;

    if (gettimeofday(&tv, NULL) == 0) {
#if ( USE_DECOUPLED_BACNET_TIME == 1 )
        // we are responsible for offset, DST, TZ
        tv.tv_sec += decoupledTimeOffset;
        tv.tv_sec += (int)Daylight_Savings_Status * 60 * 60;
        tv.tv_sec -= UTC_Offset * 60;   // note ! opposite to the classic 'Timezone is positive going east'
        tblock = gmtime((const time_t*)&tv.tv_sec);
#else
        tblock = localtime((const time_t *)&tv.tv_sec);
#endif
    }
    if (tblock) {
        status = true;
        /** struct tm
         *   int    tm_sec   Seconds [0,60].
         *   int    tm_min   Minutes [0,59].
         *   int    tm_hour  Hour [0,23].
         *   int    tm_mday  Day of month [1,31].
         *   int    tm_mon   Month of year [0,11].
         *   int    tm_year  Years since 1900.
         *   int    tm_wday  Day of week [0,6] (Sunday =0).
         *   int    tm_yday  Day of year [0,365].
         *   int    tm_isdst Daylight Savings flag.
         */
        datetime_set_date(bdate, (uint16_t)tblock->tm_year + 1900,
            (uint8_t)tblock->tm_mon + 1, (uint8_t)tblock->tm_mday);
        datetime_set_time(btime, (uint8_t)tblock->tm_hour,
            (uint8_t)tblock->tm_min, (uint8_t)tblock->tm_sec,
            (uint8_t)(tv.tv_usec / 10000));
        if (dst_active) {
            /* The value of tm_isdst is:
               - positive if Daylight Saving Time is in effect,
               - 0 if Daylight Saving Time is not in effect, and
               - negative if the information is not available. */
#if ( USE_DECOUPLED_BACNET_TIME == 1 )
            * dst_active = Daylight_Savings_Status;
#else
            if (tblock->tm_isdst > 0) {
                *dst_active = true;
            } else {
                *dst_active = false;
            }
#endif
        }
        /* note: timezone is declared in <time.h> stdlib. */
        if (utc_offset_minutes) {
            // utc_offset_minutes is the offset (east is positive)
#if ( USE_DECOUPLED_BACNET_TIME == 1 )
            * utc_offset_minutes = UTC_Offset;
#else
            /* timezone is set to the difference, in seconds,
                between Coordinated Universal Time (UTC) and
                local standard time */
            *utc_offset_minutes = timezone / 60;
#endif
        }
    }

    return status;
}


bool datetime_local_set(BACNET_DATE_TIME* bdt)
{
    struct tm tSet;
    // BACNET_DATE_TIME utcTime;
    time_t setTime;

    tSet.tm_sec = bdt->time.sec;
    tSet.tm_min = bdt->time.min;
    tSet.tm_hour = bdt->time.hour;
    tSet.tm_mday = bdt->date.day;
    tSet.tm_mon = bdt->date.month - 1;
    tSet.tm_year = bdt->date.year - 1900;

#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    // timegm expects UTC time, we use it to get our time_t and then adjust
    setTime = timegm(&tSet);
    setTime += UTC_Offset * 60 - Daylight_Savings_Status * 60 * 60 ;
    if (setTime > 0)
    {
        time_t delta = setTime - time(NULL);
        decoupledTimeOffset = delta;
    }
#else
    tSet.tm_isdst = -1;    // For OS case, let OS sort things out

    // note, you will have to run privileged to be able to do this, let's check so we don't crash!
    setTime = mktime(&tSet);  // mktime expects local returns UTC
    struct tm* tUTC = gmtime(setTime);

    if (clock_settime(CLOCK_REALTIME, tUTC) != 0)
    {
        // fail
        return false;
    }

#endif

    return true;
}


bool datetime_utc_set(BACNET_DATE_TIME* bdt)
{
    struct tm tSet;

    tSet.tm_sec = bdt->time.sec;
    tSet.tm_min = bdt->time.min;
    tSet.tm_hour = bdt->time.hour;
    tSet.tm_mday = bdt->date.day;
    tSet.tm_mon = bdt->date.month - 1;
    tSet.tm_year = bdt->date.year - 1900;
    tSet.tm_isdst = 0;  // don't care

#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    time_t setTime;
    setTime = timegm(&tSet);
    if (setTime > 0)
    {
        time_t delta = setTime - time(NULL);
        decoupledTimeOffset = delta;
    }
#else
    if (clock_settime(CLOCK_REALTIME, tSet) != 0)
    {
        // fail
        return false;
    }
#endif

    return true;
}


bool datetime_UTC_Offset_set ( int offset, BACNET_ERROR_CODE* wpEC)
{
    if ((offset < (-12 * 60)) &&
        (offset > (12 * 60)))
    {
        *wpEC = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }

#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    UTC_Offset = offset;
    return true;
#else
    * wpEC = ERROR_CODE_WRITE_PROPERTY_DENIED;
    return false;
#endif
}


int datetime_UTC_Offset_get(void)
{
#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    return UTC_Offset;
#else
    int16_t utcOffset;
    datetime_local(NULL, NULL, &utcOffset, NULL);
    return utcOffset;
#endif
}


bool datetime_DST_set(bool dst, BACNET_ERROR_CODE* wpEC)
{
#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    Daylight_Savings_Status = dst ;
    return true;
#else
    * wpEC = ERROR_CODE_WRITE_PROPERTY_DENIED;
    return false;
#endif
}


bool datetime_DST_get(void)
{
#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    return Daylight_Savings_Status;
#else
    bool dst;
    datetime_local(NULL, NULL, NULL, &dst);
    return dst;
#endif
}


/**
 * initialize the date time
 */
void datetime_init(void)
{
#if ( USE_DECOUPLED_BACNET_TIME == 1 )

    // If we decouple the time from RTC, we have maintain DST and TZ explictly. 
    // We initialize them using RTC, but for the rest of the time we will allow
    // these two properties to be written (to enable testing of Calendars, Schedules, etc.)

    struct tm* tblock = NULL;
    struct timeval tv;

    if (gettimeofday(&tv, NULL) == 0) {
        tv.tv_sec += decoupledTimeOffset;
        tblock = (struct tm*)localtime((const time_t*)&tv.tv_sec);
    }
    if (tblock) {
        /** struct tm
         *   int    tm_sec   Seconds [0,60].
         *   int    tm_min   Minutes [0,59].
         *   int    tm_hour  Hour [0,23].
         *   int    tm_mday  Day of month [1,31].
         *   int    tm_mon   Month of year [0,11].
         *   int    tm_year  Years since 1900.
         *   int    tm_wday  Day of week [0,6] (Sunday =0).
         *   int    tm_yday  Day of year [0,365].
         *   int    tm_isdst Daylight Savings flag.
         */
        Daylight_Savings_Status = (tblock->tm_isdst > 0);

        /* note: timezone is declared in <time.h> stdlib. */
        UTC_Offset = -timezone / 60;
    }
#endif
}
