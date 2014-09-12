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


class TimeConvert {
public:
    static bool Timeval2Time(const struct timeval &s, Time *d);
    static bool Timet2Time(const time_t &s, Time *d);

    static bool Time2Timeval(const Time &s, struct timeval *d);
    static bool Time2Timet(const Time &s, time_t *d);
    static bool Time2UTCString(const Time &s, std::string *d);
    static bool Time2String(const Time &s, std::string *d);

};

class Time {
public:
    static const int64 kMillisecondsPerSecond = 1000;
    static const int64 kMicrosecondsPerMillisecond = 1000;
    static const int64 kMicrosecondsPerSecond = kMicrosecondsPerMillisecond *
        kMillisecondsPerSecond;
    static const int64 kMicrosecondsPerMinute = kMicrosecondsPerSecond * 60;
    static const int64 kMicrosecondsPerHour = kMicrosecondsPerMinute * 60;
    static const int64 kMicrosecondsPerDay = kMicrosecondsPerHour * 24;
    static const int64 kMicrosecondsPerWeek = kMicrosecondsPerDay * 7;
    static const int64 kNanosecondsPerMicrosecond = 1000;
    static const int64 kNanosecondsPerSecond = kNanosecondsPerMicrosecond *
        kMicrosecondsPerSecond;

    // Represents an exploded time that can be formatted nicely. This is kind of
    // like the Win32 SYSTEMTIME structure or the Unix "struct tm" with a few
    // additions and changes to prevent errors.
    class Exploded {
    public:
        int year;          // Four digit year "2007"
        int month;         // 1-based month (values 1 = January, etc.)
        int day_of_week;   // 0-based day of week (0 = Sunday, etc.)
        int day_of_month;  // 1-based day of month (1-31)
        int hour;          // Hour within the current day (0-23)
        int minute;        // Minute within the current hour (0-59)
        int second;        // Second within the current minute (0-59 plus leap
        //   seconds which may take it up to 60).
        int millisecond;   // Milliseconds within the current second (0-999)

        // A cursory test for whether the data members are within their
        // respective ranges. A 'true' return value does not guarantee the
        // Exploded value can be successfully converted to a Time value.
        bool HasValidValues() const;
    };

    Time() : us_(0) {
    }

    bool is_null() const {
        return us_ == 0;
    }

    bool is_max() const {
        return us_ == std::numeric_limits<int64>::max();
    }

    static Time Now();
    static Time Max();
    static Time FromTimeVal(struct timeval t);
    struct timeval ToTimeVal() const;

    static Time FromUTCExploded(const Exploded &exploded) {
        return FromExploded(false, exploded);
    }

    static Time FromLocalExploded(const Exploded &exploded) {
        return FromExploded(true, exploded);
    }

    static Time FromInternalValue(int64 us) {
        return Time(us);
    }

    static Time FromTimeT(time_t tt) {
        if (tt == 0) {
            return Time();
        }
        if (tt == std::numeric_limits<time_t>::max()) {
            return Max();
        }
        return Time(tt * kMicrosecondsPerSecond);
    }

    time_t ToTimeT() const {
        if (is_null()) {
            return 0;
        }
        if (is_max()) {
            return std::numeric_limits<time_t>::max();
        }
        return us_ / kMicrosecondsPerSecond;
    }
    // Converts a string representation of time to a Time object.
    // An example of a time string which is converted is as below:-
    // "1990-10-21 2:30:49". If the timezone is not specified
    // in the input string, FromString assumes local time and FromUTCString
    // assumes UTC. A timezone that cannot be parsed (e.g. "UTC" which is not
    // specified in RFC822) is treated as if the timezone is not specified.
    // TODO(iyengar) Move the FromString/FromTimeT/ToTimeT/FromFileTime to
    // a new time converter class.
    static bool FromString(const char *time_string, Time *parsed_time) {
        return FromStringInternal(time_string, true, parsed_time);
    }

    static bool FromUTCString(const char *time_string, Time *parsed_time) {
        return FromStringInternal(time_string, false, parsed_time);
    }

    // Convert a Time to a string, which is second degree.
    // Only support one format by now, see time_string_format for details.
    bool ToStringInternal(char *time_string, int len, bool is_local);

    bool ToString(char *time_string, int len) {
        return ToStringInternal(time_string, len, true);
    }

    bool ToUTCString(char *time_string, int len) {
        return ToStringInternal(time_string, len, false);
    }

    int64 ToInternalValue() const {
        return us_;
    }

    void UTCExplode(Exploded *exploded) const {
        return Explode(false, exploded);
    }

    void LocalExplode(Exploded *exploded) const {
        return Explode(true, exploded);
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

public:
    static const char *time_string_format;

private:
    friend class TimeDelta;
    explicit Time(int64 us) : us_(us) {
    }
    void Explode(bool is_local, Exploded *exploded) const;
    static Time FromExploded(bool is_local, const Exploded &exploded);

    // Converts a string representation of time to a Time object.
    // An example of a time string which is converted is as below:-
    // "Tue, 15 Nov 1994 12:45:26 GMT". If the timezone is not specified
    // in the input string, local time |is_local = true| or
    // UTC |is_local = false| is assumed. A timezone that cannot be parsed
    // (e.g. "UTC" which is not specified in RFC822) is treated as if the
    // timezone is not specified.
    static bool FromStringInternal(const char *time_string,
                                   bool is_local,
                                   Time *parsed_time);
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

    static TimeTicks UnixEpoch() {
        // return TickTicks::Now() - Time::Now();
    }

    int64 ToInternalValue() const {
        return ticks_;
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
