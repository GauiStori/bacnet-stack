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

#include <time.h>

#include "bacnet/datetime.h"

#ifndef USE_DECOUPLED_BACNET_TIME
#error - ensure this gets set to 0 or 1 in datetime.h
#endif 


#if ( USE_DECOUPLED_BACNET_TIME == 0 )
#include <error.h>
#include <errno.h>
#include <string.h>
#include "bacnet/bacenum.h"
#endif


#if ( USE_DECOUPLED_BACNET_TIME == 1 )

// todo - you may want to persist this value
static time_t decoupledTimeOffset_seconds;

static bool UTC_Offset_override;
static int32_t UTC_Offset_seconds;

static bool Daylight_Savings_Status_override;
static bool Daylight_Savings_Status;

#endif 


// todo 1 - once a full pass has been made with other implementations, optimize this (lots of redundancies)

static bool datetime_isDST(time_t localTime)
{
    struct tm* tblock = localtime(&localTime);
    return (tblock->tm_isdst > 0) ? true : false;
}


static int16_t datetime_UTCOffsetMinutes( void )
{
    time_t gmttime = time(NULL);
    struct tm* tblock = localtime(&gmttime);
    if (tblock->tm_isdst)
    {
        return (int16_t) ( - tblock->tm_gmtoff / 60 + 60 ) ;
    }
    else
    {
        return (int16_t) (- tblock->tm_gmtoff / 60);
    }
}


static bool datetime_local_raw(
    BACNET_DATE* bdate,
    BACNET_TIME* btime,
    int16_t* utc_offset_minutes,
    bool* dst_active)
{
    bool status = false;
//    bool dst;
//    int tUTC_offset;

    time_t tSinceEpoch_seconds = time(NULL);

#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    // we are responsible for offset, but in this raw call, not DST or UTC_offset
    tSinceEpoch_seconds += decoupledTimeOffset_seconds;
#endif

    struct tm *tblock = localtime(&tSinceEpoch_seconds);

    int utcOffsetMinutes = datetime_UTCOffsetMinutes() ;

    if (tblock)
    {
        status = true;
         // todo 1 - check for null pointer indicating we don't want this
        datetime_set_date(
            bdate, 
            (uint16_t) (tblock->tm_year + 1900),
            (uint8_t) (tblock->tm_mon + 1), 
            (uint8_t)tblock->tm_mday);
        datetime_set_time(btime, (uint8_t)tblock->tm_hour,
            (uint8_t)tblock->tm_min, (uint8_t)tblock->tm_sec, 0);

        if (dst_active)
        {
            /* The value of tm_isdst is:
               - positive if Daylight Saving Time is in effect,
               - 0 if Daylight Saving Time is not in effect, and
               - negative if the information is not available. */
            *dst_active = (tblock->tm_isdst > 0) ? true : false;
        }

        /* note: timezone is declared in <time.h> stdlib. */
        if (utc_offset_minutes) {
            // utc_offset_minutes is the offset (east is positive)
            *utc_offset_minutes = (int16_t)( utcOffsetMinutes);
        }
    }
    else
    {
        if (bdate)
        {
            datetime_set_date(bdate, 1900, 1, 1);
        }
        if (btime)
        {
            datetime_set_time(btime, 0, 0, 0, 0);
        }
        if (dst_active)
        {
            *dst_active = false;
        }
        if (utc_offset_minutes)
        {
            *utc_offset_minutes = 8 * 60;
        }
        // panic();
    }

    return status;
}



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
    BACNET_DATE* bdate,
    BACNET_TIME* btime,
    int16_t* utc_offset_minutes,
    bool* dst_active)
{
    bool status = false;
    bool dst;
    struct tm* tblock = NULL;

    time_t tSinceEpoch_seconds = time(NULL);

#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    
    int32_t tUTC_offsetSeconds;

    // we are responsible for offset, DST, TZ
    tSinceEpoch_seconds += decoupledTimeOffset_seconds;
    if (Daylight_Savings_Status_override || UTC_Offset_override)
    {
        // UpdateDSTandUTCoffset(tSinceEpoch_seconds);
    }

    if (UTC_Offset_override)
    {
        tUTC_offsetSeconds = UTC_Offset_seconds;
    }
    else
    {
        int16_t utcOffsetMinutes = datetime_UTCOffsetMinutes();
        tUTC_offsetSeconds = utcOffsetMinutes * 60 ;
    }

    tSinceEpoch_seconds -= tUTC_offsetSeconds;   // note ! opposite to the classic 'Timezone is positive going east'
    if (Daylight_Savings_Status_override)
    {
        tSinceEpoch_seconds += (Daylight_Savings_Status) ? 60 * 60 : 0;
        dst = Daylight_Savings_Status;
    }
    else
    {
        if (datetime_isDST(tSinceEpoch_seconds))
        {
            tSinceEpoch_seconds += 60 * 60 ;
            dst = true;
        }
        else
        {
            dst = false;
        }
    }


    tblock = gmtime(&tSinceEpoch_seconds);
    if (tblock)
    {
        tblock->tm_isdst = dst;
#else
    tblock = localtime(&tSinceEpoch_seconds);
    if (tblock)
    {
#endif
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
         // todo 1 - check for null pointer indicating we don't want this
        datetime_set_date(
            bdate, 
            (uint16_t)(tblock->tm_year + 1900),
            (uint8_t)(tblock->tm_mon + 1), 
            (uint8_t)tblock->tm_mday);

        datetime_set_time(
            btime, 
            (uint8_t)tblock->tm_hour,
            (uint8_t)tblock->tm_min, 
            (uint8_t)tblock->tm_sec, 
            0);
        // todo 1 (uint8_t)(tv.tv_usec / 10000));

        if (dst_active)
        {
            /* The value of tm_isdst is:
               - positive if Daylight Saving Time is in effect,
               - 0 if Daylight Saving Time is not in effect, and
               - negative if the information is not available. */
            *dst_active = (tblock->tm_isdst > 0) ? true : false;
        }

        /* note: timezone is declared in <time.h> stdlib. */
        if (utc_offset_minutes) {
            *utc_offset_minutes = datetime_UTCOffsetMinutes();
        }
    }
    else
    {
        datetime_set_date(bdate, 1900, 1, 1);
        datetime_set_time(btime, 0, 0, 0, 0);
        // panic();
    }

    return status;
}


bool datetime_local_set(BACNET_DATE_TIME* bdt)
{
    struct tm tSet;
    tSet.tm_sec = bdt->time.sec;
    tSet.tm_min = bdt->time.min;
    tSet.tm_hour = bdt->time.hour;
    tSet.tm_mday = bdt->date.day;
    tSet.tm_mon = bdt->date.month - 1;
    tSet.tm_year = bdt->date.year - 1900;

#if ( USE_DECOUPLED_BACNET_TIME == 1 )

    // One of the first things we do is cancel any overrides
    Daylight_Savings_Status_override = false;
    UTC_Offset_override = false;

    // timegm expects UTC time, we use it to get our time_t and then adjust
    time_t setTime = timegm(&tSet);
    time_t gmTimeNow = time(NULL);

    if (setTime > 0)
    {
        int16_t utcOffsetMinutes = datetime_UTCOffsetMinutes() ;
        bool dst = datetime_isDST(setTime);

        setTime += (time_t) utcOffsetMinutes * 60 ;

        if ( dst )
        {
            setTime -= 60 * 60 ;
        }

        time_t delta = setTime - gmTimeNow;
        decoupledTimeOffset_seconds = delta;
    }
#else
    tSet.tm_isdst = -1;    // For OS case, let OS sort things out

    // note, you will have to run privileged to be able to do this, we should check so we don't crash!
    time_t setTime = mktime(&tSet);  // mktime expects local returns UTC
    
    struct timespec newTime;
    newTime.tv_sec = setTime;
    newTime.tv_nsec = 0;

    if (clock_settime(CLOCK_REALTIME, &newTime ) != 0)
    {
        // fail - most likely error is app is not run with suitable (root) priviliges
        // and since this gets called from a unconfirmed message, there is no way to warn anyone, except to panic
        return false;
    }
#endif

    return true;
}


bool datetime_utc_set(BACNET_DATE_TIME * bdt)
{
    struct tm tSet;

    tSet.tm_sec = bdt->time.sec;
    tSet.tm_min = bdt->time.min;
    tSet.tm_hour = bdt->time.hour;
    tSet.tm_mday = bdt->date.day;
    tSet.tm_mon = bdt->date.month - 1;
    tSet.tm_year = bdt->date.year - 1900;
    tSet.tm_isdst = 0;  // don't care - we are going to do UTC operations

    time_t setTime;
    setTime = timegm(&tSet);
    if (setTime > 0)
    {
#if ( USE_DECOUPLED_BACNET_TIME == 1 )
        time_t delta = setTime - time(NULL);
        decoupledTimeOffset_seconds = delta ;
#else
        struct timespec tspec;
        tspec.tv_sec = setTime;
        tspec.tv_nsec = 0;

        if (clock_settime(CLOCK_REALTIME, &tspec) != 0)
        {
            // fail - most likely error is app is not run with suitable (root) priviliges
            // and since this gets called from a unconfirmed message, there is no way to warn anyone, except to panic
            return false;
        }
#endif
    }

    return true;
}


bool datetime_UTC_Offset_set(int offset, BACNET_ERROR_CODE * wpEC)
{
    if ((offset < (-12 * 60)) &&
        (offset > (12 * 60)))
    {
        *wpEC = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }

#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    UTC_Offset_seconds = offset * 60 ;
    UTC_Offset_override = true;
    return true;
#else
    * wpEC = ERROR_CODE_WRITE_ACCESS_DENIED;
    return false;
#endif
}


int datetime_UTC_Offset_get(void)
{
#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    if (UTC_Offset_override)
    {
        return UTC_Offset_seconds / 60 ;
    }
    else
    {
        int16_t tUTCoffsetMinutes;
        datetime_local_raw(NULL, NULL, &tUTCoffsetMinutes, NULL);
        return tUTCoffsetMinutes;
    }
#else
    int16_t utcOffset;
    datetime_local(NULL, NULL, &utcOffset, NULL);
    return utcOffset;
#endif
}


bool datetime_DST_set(bool dst, BACNET_ERROR_CODE * wpEC)
{
#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    (void)wpEC;
    Daylight_Savings_Status = dst;
    Daylight_Savings_Status_override = true;
    return true;
#else
    * wpEC = ERROR_CODE_WRITE_ACCESS_DENIED;
    return false;
#endif
}


bool datetime_DST_get(void)
{
#if ( USE_DECOUPLED_BACNET_TIME == 1 )
    if (Daylight_Savings_Status_override)
    {
        return Daylight_Savings_Status;
    }
    else
    {
        time_t utcTime = time(NULL);
        time_t adjTime = utcTime + decoupledTimeOffset_seconds;
        adjTime += -datetime_UTC_Offset_get() * 60;
        bool dst = datetime_isDST(adjTime);
        return dst;
    }
#else
    bool dst;
    datetime_local(NULL, NULL, NULL, &dst);
    return dst;
#endif
}


/**
 * initialize the date time if necessary
 */
void datetime_init(void)
{
}
