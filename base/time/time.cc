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

// TimeDelta
// static function
TimeDelta TimeDelta::FromDays(int days) {
    if (days == std::numeric_limits<int>::max()) {
        return Max();
    }
    return TimeDelta(days * kMicrosecondsPerDay);
}

// static function
TimeDelta TimeDelta::FromHours(int hours) {
    if (hours == std::numeric_limits<int>::max()) {
        return Max();
    }
    return TimeDelta(hours * kMicrosecondsPerHour);
}

// static function
TimeDelta TimeDelta::FromMinutes(int minutes) {
    if (minutes == std::numeric_limits<int>::max()) {
        return Max();
    }
    return TimeDelta(minutes * kMicrosecondsPerMinute);
}

int TimeDelta::InDays() const {
    if (is_max()) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(delta_ / kMicrosecondsPerDay);
}

int TimeDelta::InHours() const {
    if (is_max()) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(delta_ / kMicrosecondsPerHour);
}

int TimeDelta::InMinutes() const {
    if (is_max()) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(delta_ / kMicrosecondsPerMinute);
}


// Time
// static function
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

// static function
Time Time::Max()
{
    return Time(std::numeric_limits<int64>::max());
}



// TimeConverter
// static function
time_t TimeConverter::TimeStruct2TimeT(struct tm &time_struct,
                                       bool is_local)
{
    if (is_local)
        return mktime(&time_struct);
    else
        return timegm(&time_struct);
}

// static function
void TimeConverter::TimeT2TimeStruct(const time_t &t,
                                     struct tm *time_struct_ptr,
                                     bool is_local)
{
    if (is_local)
        localtime_r(&t, time_struct_ptr);
    else
        gmtime_r(&t, time_struct_ptr);
}

// static function
Time TimeConverter::TimeVal2Time(const struct timeval &t) {
    DCHECK_LT(t.tv_usec, static_cast<int>(kMicrosecondsPerSecond));
    DCHECK_GE(t.tv_usec, 0);
    if (t.tv_usec == 0 && t.tv_sec == 0)
        return Time();
    if (t.tv_usec ==
        static_cast<suseconds_t>(kMicrosecondsPerSecond) - 1 &&
        t.tv_sec == std::numeric_limits<time_t>::max())
        return Time::Max();
    return Time::FromInternalValue(static_cast<int64>(t.tv_sec) *
                                   kMicrosecondsPerSecond);
}

// static function
struct timeval TimeConverter::Time2TimeVal(const Time &t) {
    struct timeval result;
    if (t.is_null()) {
        result.tv_sec = 0;
        result.tv_usec = 0;
        return result;
    }
    if (t.is_max()) {
        result.tv_sec = std::numeric_limits<time_t>::max();
        result.tv_usec =
                static_cast<suseconds_t>(kMicrosecondsPerSecond) - 1;
        return result;
    }
    int64 us = t.ToInternalValue();
    result.tv_sec = us / kMicrosecondsPerSecond;
    result.tv_usec = us % kMicrosecondsPerSecond;
    return result;
}

// static function
const char *TimeConverter::time_string_format = "%Y-%m-%d %H:%M:%S";
std::string TimeConverter::Time2String(const Time &t, bool is_local)
{
    struct tm timeinfo;
    char time_string[1024];
    memset(&timeinfo, 0, sizeof(timeinfo));
    TimeT2TimeStruct(Time2TimeT(t), &timeinfo, is_local);
    strftime(time_string, 1024, time_string_format, &timeinfo);
    return std::string(time_string);
}

// static function
Time TimeConverter::String2Time(const std::string &time_string, bool is_local)
{
    if (time_string[0] == '\0') {
        return Time();
    }
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(timeinfo));
    strptime(time_string.c_str(), time_string_format, &timeinfo);
    time_t tt = TimeStruct2TimeT(timeinfo, is_local);
    return TimeT2Time(tt);
}

// static function
Time TimeConverter::TimeT2Time(const time_t &tt) {
    if (tt == 0) {
        return Time();
    }
    if (tt == std::numeric_limits<time_t>::max()) {
        return Time::Max();
    }
    return Time::FromInternalValue(tt * kMicrosecondsPerSecond);
}

// static function
time_t TimeConverter::Time2TimeT(const Time &t) {
    if (t.is_null()) {
        return 0;
    }
    if (t.is_max()) {
        return std::numeric_limits<time_t>::max();
    }
    return t.ToInternalValue() / kMicrosecondsPerSecond;
}

// TimeTicks
TimeTicks ClockNow(clockid_t id)
{
    uint64_t absolute_micro;
    struct timespec ts;
    if (clock_gettime(id, &ts) !=0) {
        return TimeTicks();
    }

    absolute_micro =
            (static_cast<int64>(ts.tv_sec) * kMicrosecondsPerSecond) +
            (static_cast<int64>(ts.tv_nsec) / kNanosecondsPerMicrosecond);
    return TimeTicks::FromInternalValue(absolute_micro);
}

// static function
TimeTicks TimeTicks::Now()
{
    return ClockNow(CLOCK_MONOTONIC);
}

}  // namespace base
