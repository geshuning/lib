// Copyright (c) 2014 Shuning Ge

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "base/time/time.hh"
#include "base/synchronization/lock.hh"
#include "base/logging/logging.hh"

namespace base {

time_t SysTimeFromTimeStruct(struct tm* timestruct, bool is_local)
{
    if (is_local)
        return mktime(timestruct);
    else
        return timegm(timestruct);
}

void SysTimeToTimeStruct(time_t t, struct tm* timestruct, bool is_local)
{
    if (is_local)
        localtime_r(&t, timestruct);
    else
        gmtime_r(&t, timestruct);
}

// static function
TimeDelta TimeDelta::FromDays(int days) {
    if (days == std::numeric_limits<int>::max()) {
        return Max();
    }
    return TimeDelta(days * Time::kMicrosecondsPerDay);
}

// static function
TimeDelta TimeDelta::FromHours(int hours) {
    if (hours == std::numeric_limits<int>::max()) {
        return Max();
    }
    return TimeDelta(hours * Time::kMicrosecondsPerHour);
}

// static function
TimeDelta TimeDelta::FromMinutes(int minutes) {
    if (minutes == std::numeric_limits<int>::max()) {
        return Max();
    }
    return TimeDelta(minutes * Time::kMicrosecondsPerMinute);
}

int TimeDelta::InDays() const {
    if (is_max()) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(delta_ / Time::kMicrosecondsPerDay);
}

int TimeDelta::InHours() const {
    if (is_max()) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(delta_ / Time::kMicrosecondsPerHour);
}

int TimeDelta::InMinutes() const {
    if (is_max()) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(delta_ / Time::kMicrosecondsPerMinute);
}

Time Time::Now()
{
    struct timeval tv;
    struct timezone tz = {0, 0};
    if (gettimeofday(&tv, &tz) != 0) {
        DCHECK(0) << "Could not determine time of day";
        return Time();
    }
    return Time((tv.tv_sec * kMicrosecondsPerSecond + tv.tv_usec));
}

Time Time::Max()
{
    return Time(std::numeric_limits<int64>::max());
}

Time Time::FromTimeVal(struct timeval t) {
    DCHECK_LT(t.tv_usec, static_cast<int>(Time::kMicrosecondsPerSecond));
    DCHECK_GE(t.tv_usec, 0);
    if (t.tv_usec == 0 && t.tv_sec == 0)
        return Time();
    if (t.tv_usec ==
        static_cast<suseconds_t>(Time::kMicrosecondsPerSecond) - 1 &&
        t.tv_sec == std::numeric_limits<time_t>::max())
        return Max();
    return Time(static_cast<int64>(t.tv_sec) * Time::kMicrosecondsPerSecond);
}

struct timeval Time::ToTimeVal() const {
    struct timeval result;
    if (is_null()) {
        result.tv_sec = 0;
        result.tv_usec = 0;
        return result;
    }
    if (is_max()) {
        result.tv_sec = std::numeric_limits<time_t>::max();
        result.tv_usec =
                static_cast<suseconds_t>(Time::kMicrosecondsPerSecond) - 1;
        return result;
    }
    int64 us = us_;
    result.tv_sec = us / Time::kMicrosecondsPerSecond;
    result.tv_usec = us % Time::kMicrosecondsPerSecond;
    return result;
}

void Time::Explode(bool is_local, Exploded* exploded) const {
    int64 microseconds = us_;
    int64 milliseconds;  // Milliseconds since epoch.
    time_t seconds;  // Seconds since epoch.
    int millisecond;  // Exploded millisecond value (0-999).
    if (microseconds >= 0) {
        // Rounding towards -infinity <=> rounding towards 0, in this case.
        milliseconds = microseconds / kMicrosecondsPerMillisecond;
        seconds = milliseconds / kMillisecondsPerSecond;
        millisecond = milliseconds % kMillisecondsPerSecond;
    } else {
        // Round these *down* (towards -infinity).
        milliseconds = (microseconds - kMicrosecondsPerMillisecond + 1) /
                kMicrosecondsPerMillisecond;
        seconds = (milliseconds - kMillisecondsPerSecond + 1) /
                kMillisecondsPerSecond;
        // Make this nonnegative (and between 0 and 999 inclusive).
        millisecond = milliseconds % kMillisecondsPerSecond;
        if (millisecond < 0)
            millisecond += kMillisecondsPerSecond;
    }

    struct tm timestruct;
    SysTimeToTimeStruct(seconds, &timestruct, is_local);

    exploded->year         = timestruct.tm_year + 1900;
    exploded->month        = timestruct.tm_mon + 1;
    exploded->day_of_week  = timestruct.tm_wday;
    exploded->day_of_month = timestruct.tm_mday;
    exploded->hour         = timestruct.tm_hour;
    exploded->minute       = timestruct.tm_min;
    exploded->second       = timestruct.tm_sec;
    exploded->millisecond  = millisecond;
}

Time Time::FromExploded(bool is_local, const Exploded &exploded)
{
    struct tm timestruct;
    timestruct.tm_sec    = exploded.second;
    timestruct.tm_min    = exploded.minute;
    timestruct.tm_hour   = exploded.hour;
    timestruct.tm_mday   = exploded.day_of_month;
    timestruct.tm_mon    = exploded.month - 1;
    timestruct.tm_year   = exploded.year - 1900;
    timestruct.tm_wday   = exploded.day_of_week;  // mktime/timegm ignore this
    timestruct.tm_yday   = 0;     // mktime/timegm ignore this
    timestruct.tm_isdst  = -1;    // attempt to figure it out
    timestruct.tm_gmtoff = 0;     // not a POSIX field, so mktime/timegm ignore
    timestruct.tm_zone   = NULL;  // not a POSIX field, so mktime/timegm ignore

    int64 milliseconds;
    time_t seconds;

    // Certain exploded dates do not really exist due to daylight saving times,
    // and this causes mktime() to return implementation-defined values when
    // tm_isdst is set to -1. On Android, the function will return -1, while the
    // C libraries of other platforms typically return a liberally-chosen value.
    // Handling this requires the special code below.

    // SysTimeFromTimeStruct() modifies the input structure, save current value.
    struct tm timestruct0 = timestruct;

    seconds = SysTimeFromTimeStruct(&timestruct, is_local);
    if (seconds == -1) {
        // Get the time values with tm_isdst == 0 and 1, then select
        // the closest one to UTC 00:00:00 that isn't -1.
        timestruct = timestruct0;
        timestruct.tm_isdst = 0;
        int64 seconds_isdst0 = SysTimeFromTimeStruct(&timestruct, is_local);

        timestruct = timestruct0;
        timestruct.tm_isdst = 1;
        int64 seconds_isdst1 = SysTimeFromTimeStruct(&timestruct, is_local);

        // seconds_isdst0 or seconds_isdst1 can be -1 for some timezones.
        // E.g. "CLST" (Chile Summer Time) returns -1 for 'tm_isdt == 1'.
        if (seconds_isdst0 < 0)
            seconds = seconds_isdst1;
        else if (seconds_isdst1 < 0)
            seconds = seconds_isdst0;
        else
            seconds = std::min(seconds_isdst0, seconds_isdst1);
    }

    // Handle overflow.  Clamping the range to what mktime and timegm might
    // return is the best that can be done here. It's not ideal, but it's better
    // than failing here or ignoring the overflow case and treating each time
    // overflow as one second prior to the epoch.
    if (seconds == -1 &&
        (exploded.year < 1969 || exploded.year > 1970)) {
        // If exploded.year is 1969 or 1970, take -1 as correct, with the
        // time indicating 1 second prior to the epoch.(1970 is allowed to
        // handle time zone and DST offsets.)
        // Otherwise, return the most future or past time representable.
        // Assumes the time_t epoch is 1970-01-01 00:00:00 UTC.
        //
        // The minimum and maximum representible times that mktime and timegm
        // could return are used here instead of values outside that range to
        // allow proper round-tripping between exploded and counter-type time
        // representations in the presence of possible truncation to time_t by
        // division and use with other functions that accept time_t.
        //
        // When representing the most distant time in the future, add an extra
        // 999ms to avoid the time being less than any other possible value that
        // this function can return.
        const int64 min_seconds = (sizeof(time_t) < sizeof(int64))
                ? std::numeric_limits<time_t>::min()
                : std::numeric_limits<int32_t>::min();
        const int64 max_seconds = (sizeof(time_t) < sizeof(int64))
                ? std::numeric_limits<time_t>::max()
                : std::numeric_limits<int32_t>::max();
        if (exploded.year < 1969) {
            milliseconds = min_seconds * kMillisecondsPerSecond;
        } else {
            milliseconds = max_seconds * kMillisecondsPerSecond;
            milliseconds += (kMillisecondsPerSecond - 1);
        }
    } else {
        milliseconds = seconds * kMillisecondsPerSecond + exploded.millisecond;
    }
    return Time(milliseconds * kMicrosecondsPerMillisecond);
}

bool is_in_range(int value, int lo, int hi)
{
    return value >= lo && value <= hi;
}
bool Time::Exploded::HasValidValues() const
{
    return  is_in_range(month, 1, 12) &&
            is_in_range(day_of_week, 0, 6) &&
            is_in_range(day_of_month, 1, 31) &&
            is_in_range(hour, 0, 23) &&
            is_in_range(minute, 0, 59) &&
            is_in_range(second, 0, 59) &&
            is_in_range(millisecond, 0, 999);
}

const char *Time::time_string_format = "%Y-%m-%d %H:%M:%S";
// static
bool Time::FromStringInternal(const char *time_string,
                              bool is_local,
                              Time *parsed_time)
{
    DCHECK((time_string != NULL) && (parsed_time != NULL));
    if (time_string[0] == '\0') {
        return false;
    }
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(timeinfo));
    strptime(time_string, time_string_format, &timeinfo);
    time_t tt = SysTimeFromTimeStruct(&timeinfo, is_local);
    *parsed_time = FromTimeT(tt);
    return true;
}

bool Time::ToStringInternal(char *time_string, int len, bool is_local)
{
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(timeinfo));
    SysTimeToTimeStruct(ToTimeT(), &timeinfo, is_local);
    strftime(time_string, len, time_string_format, &timeinfo);
    return true;
}

TimeTicks ClockNow(clockid_t id)
{
    uint64_t absolute_micro;
    struct timespec ts;
    if (clock_gettime(id, &ts) !=0) {
        return TimeTicks();
    }

    absolute_micro =
            (static_cast<int64>(ts.tv_sec) * Time::kMicrosecondsPerSecond) +
            (static_cast<int64>(ts.tv_nsec) / Time::kNanosecondsPerMicrosecond);
    return TimeTicks::FromInternalValue(absolute_micro);
}

// static
TimeTicks TimeTicks::Now()
{
    return ClockNow(CLOCK_MONOTONIC);
}

}  // namespace base
