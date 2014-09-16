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

#ifndef BASE_TIME_TIME_HH_
#define BASE_TIME_TIME_HH_

#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <limits>
#include <string>

#include <base/basictypes.hh>

namespace base {


const int64 kMillisecondsPerSecond = 1000;
const int64 kMicrosecondsPerMillisecond = 1000;
const int64 kMicrosecondsPerSecond = kMicrosecondsPerMillisecond *
        kMillisecondsPerSecond;
const int64 kMicrosecondsPerMinute = kMicrosecondsPerSecond * 60;
const int64 kMicrosecondsPerHour = kMicrosecondsPerMinute * 60;
const int64 kMicrosecondsPerDay = kMicrosecondsPerHour * 24;
const int64 kMicrosecondsPerWeek = kMicrosecondsPerDay * 7;
const int64 kNanosecondsPerMicrosecond = 1000;
const int64 kNanosecondsPerSecond = kNanosecondsPerMicrosecond *
        kMicrosecondsPerSecond;

class Time;
class TimeTicks;

class TimeDelta {
public:
    TimeDelta() : delta_(0) {}

    static TimeDelta FromDays(int days);
    static TimeDelta FromHours(int hours);
    static TimeDelta FromMinutes(int minutes);

    static TimeDelta Max() {
        return TimeDelta(std::numeric_limits<int64>::max());
    }

    bool is_max() const {
        return delta_ == std::numeric_limits<int64>::max();
    }

    int InDays() const;
    int InHours() const;
    int InMinutes() const;

    TimeDelta &operator=(TimeDelta other) {
        delta_ = other.delta_;
        return *this;
    }

    TimeDelta operator+(TimeDelta other) const {
        return TimeDelta(delta_ + other.delta_);
    }

    TimeDelta operator-(TimeDelta other) const {
        return TimeDelta(delta_ - other.delta_);
    }

    TimeDelta &operator+=(TimeDelta other) {
        delta_ += other.delta_;
        return *this;
    }

    TimeDelta &operator-=(TimeDelta other) {
        delta_ -= other.delta_;
        return *this;
    }

    bool operator==(TimeDelta other) const {
        return delta_ == other.delta_;
    }

    bool operator!=(TimeDelta other) const {
        return delta_ != other.delta_;
    }

    bool operator<(TimeDelta other) const {
        return delta_ < other.delta_;
    }

    bool operator<=(TimeDelta other) const {
        return delta_ <= other.delta_;
    }

    bool operator>(TimeDelta other) const {
        return delta_ > other.delta_;
    }

    bool operator>=(TimeDelta other) const {
        return delta_ >= other.delta_;
    }

private:
    friend class Time;
    friend class TimeTicks;

    // Constructs a delta given the duration in microseconds. This is private
    // to avoid confusion by callers with an integer constructor. Use
    // FromSeconds, FromMilliseconds, etc. instead.
    explicit TimeDelta(int64 delta_us) : delta_(delta_us) {
    }

    int64 delta_;
};


class TimeConverter {
public:
    static Time TimeVal2Time(const struct timeval &val);
    static Time TimeT2Time(const time_t &val);

    static struct timeval Time2TimeVal(const Time &val);
    static time_t Time2TimeT(const Time &val);
    static std::string Time2String(const Time &val, bool is_local);
    static Time String2Time(const std::string &s, bool is_local);

    static void TimeT2TimeStruct(const time_t &val,
                                 struct tm *time_struct,
                                 bool is_local);
    static time_t TimeStruct2TimeT(struct tm &time_struct, bool is_local);

public:
    static const char *time_string_format;
};

class Time {
public:
    Time() : us_(0) {}

    bool is_null() const {
        return us_ == 0;
    }

    bool is_max() const {
        return us_ == std::numeric_limits<int64>::max();
    }

    static Time Now();
    static Time Max();
    static Time FromInternalValue(int64 us) {
        return Time(us);
    }

    int64 ToInternalValue() const {
        return us_;
    }

    Time &operator=(Time other) {
        us_ = other.us_;
        return *this;
    }

    TimeDelta operator-(Time other) const {
        return TimeDelta(us_ - other.us_);
    }

    Time &operator+=(TimeDelta delta) {
        us_ += delta.delta_;
        return *this;
    }

    Time &operator-=(TimeDelta delta) {
        us_ -= delta.delta_;
        return *this;
    }

    Time operator+(TimeDelta delta) const {
        return Time(us_ + delta.delta_);
    }

    Time operator-(TimeDelta delta) const {
        return Time(us_ - delta.delta_);
    }

    bool operator==(Time other) const {
        return us_ == other.us_;
    }

    bool operator!=(Time other) const {
        return us_ != other.us_;
    }
    bool operator>(Time other) const {
        return us_ > other.us_;
    }
    bool operator>=(Time other) const {
        return us_ >= other.us_;
    }
    bool operator<(Time other) const {
        return us_ < other.us_;
    }
    bool operator<=(Time other) const {
        return us_ <= other.us_;
    }


private:
    friend class TimeDelta;
    explicit Time(int64 us) : us_(us) {}
    int64 us_;
};

class TimeTicks {
public:
    static const clockid_t kClockSystemTrace = 11;
    TimeTicks() : ticks_(0) {}
    static TimeTicks Now();
    bool is_null() const {
        return ticks_ == 0;
    }

    static TimeTicks FromInternalValue(int64 ticks) {
        return TimeTicks(ticks);
    }

    int64 ToInternalValue() const {
        return ticks_;
    }

    static TimeTicks UnixEpoch() {
        // return TickTicks::Now() - Time::Now();
    }

    TimeTicks &operator=(TimeTicks other) {
        ticks_ = other.ticks_;
        return *this;
    }

    TimeDelta operator-(TimeTicks other) const {
        return TimeDelta(ticks_ - other.ticks_);
    }

    TimeTicks &operator+=(TimeDelta delta) {
        ticks_ += delta.delta_;
        return *this;
    }

    TimeTicks &operator-=(TimeDelta delta) {
        ticks_ -= delta.delta_;
        return *this;
    }

    TimeTicks operator+(TimeDelta delta) const {
        return TimeTicks(ticks_ + delta.delta_);
    }

    TimeTicks operator-(TimeDelta delta) const {
        return TimeTicks(ticks_ - delta.delta_);
    }

    bool operator==(TimeTicks other) const {
        return ticks_ == other.ticks_;
    }

    bool operator!=(TimeTicks other) const {
        return ticks_ != other.ticks_;
    }

    bool operator<(TimeTicks other) const {
        return ticks_ < other.ticks_;
    }

    bool operator<=(TimeTicks other) const {
        return ticks_ <= other.ticks_;
    }

    bool operator>(TimeTicks other) const {
        return ticks_ > other.ticks_;
    }

    bool operator>=(TimeTicks other) const {
        return ticks_ >= other.ticks_;
    }

private:
    friend class TimeDelta;
    explicit TimeTicks(int64 ticks) : ticks_(ticks) {}

    int64 ticks_;
};


}      // namespace base
#endif  // BASE_TIME_TIME_HH
