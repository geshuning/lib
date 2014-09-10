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

namespace base {
typedef time_t SysTime;
SysTime SysTimeFromTimeStruct(struct tm* timestruct, bool is_local)
{
    // base::AutoLock locked(g_sys_time_to_time_struct_lock.Get());
    if (is_local)
        return mktime(timestruct);
    else
        return timegm(timestruct);
}

SysTime SysTimeToTimeStruct(time_t t, struct tm* timestruct, bool is_local)
{
    // base::AutoLock locked(g_sys_time_to_time_struct_lock.Get());
    if (is_local)
        localtime_r(&t, timestruct);
    else
        gmtime_r(&t, timestruct);
}

// Static functions
TimeDelta TimeDelta::FromDays(int days) {
    if (days == std::numeric_limits<int>::max()) {
        return Max();
    }
    return TimeDelta(days * Time::kMicrosecondsPerDay);
}

TimeDelta TimeDelta::FromHours(int hours) {
    if (hours == std::numeric_limits<int>::max()) {
        return Max();
    }
    return TimeDelta(hours * Time::kMicrosecondsPerHour);
}

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

Time Time::UnixEpoch()
{
    Time time;
    time.us_ = kTimeTToMicrosecondsOffset;
    return time;
}

Time Time::Now()
{
    struct timeval tv;
    struct timezone tz = {0, 0};
    if (gettimeofday(&tv, &tz) != 0) {
        // TODO(gene.ge):add log
        return Time();
    }
    // Combine seconds and microseconds in a 64-bit
    // return Time((tv.tv_sec * kMicrosecondsPerSecond + tv.tv_usec) +
    //             kWindowsEpochDeltaMicroseconds);

    return Time((tv.tv_sec * kMicrosecondsPerSecond + tv.tv_usec));
}

Time Time::Max()
{
    return Time(std::numeric_limits<int64>::max());
}

Time Time::FromTimeVal(struct timeval t) {
    // DCHECK_LT(t.tv_usec, static_cast<int>(Time::kMicrosecondsPerSecond));
    // DCHECK_GE(t.tv_usec, 0);
    if (t.tv_usec == 0 && t.tv_sec == 0)
        return Time();
    if (t.tv_usec ==
        static_cast<suseconds_t>(Time::kMicrosecondsPerSecond) - 1 &&
        t.tv_sec == std::numeric_limits<time_t>::max())
        return Max();
    return Time(
        (static_cast<int64>(t.tv_sec) * Time::kMicrosecondsPerSecond) +
              t.tv_usec +
        kTimeTToMicrosecondsOffset);
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
    int64 us = us_ - kTimeTToMicrosecondsOffset;
    result.tv_sec = us / Time::kMicrosecondsPerSecond;
    result.tv_usec = us % Time::kMicrosecondsPerSecond;
    return result;
}

void Time::Explode(bool is_local, Exploded* exploded) const {
    // Time stores times with microsecond resolution, but Exploded only carries
    // millisecond resolution, so begin by being lossy.  Adjust from Windows
    // epoch (1601) to Unix epoch (1970);
    // int64 microseconds = us_ - kWindowsEpochDeltaMicroseconds;
    int64 microseconds = 0;
    // The following values are all rounded towards -infinity.
    int64 milliseconds;  // Milliseconds since epoch.
    SysTime seconds;  // Seconds since epoch.
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
    return Time(0);
}

}  // namespace base
