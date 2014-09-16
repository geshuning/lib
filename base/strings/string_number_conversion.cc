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

#include "base/strings/string_number_conversion.hh"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include <limits>

#include "base/logging/logging.hh"

namespace base {
namespace {

template <typename STR, typename INT, typename UINT, bool NEG>
struct IntToStringT {
    // This is to avoid a compiler warning about unary minus on unsigned type.
    // For example, say you had the following code:
    //   template <typename INT>
    //   INT abs(INT value) { return value < 0 ? -value : value; }
    // Even though if INT is unsigned, it's impossible for value < 0, so the
    // unary minus will never be taken, the compiler will still generate a
    // warning.  We do a little specialization dance...
    template <typename INT2, typename UINT2, bool NEG2>
    struct ToUnsignedT {};

    template <typename INT2, typename UINT2>
    struct ToUnsignedT<INT2, UINT2, false> {
        static UINT2 ToUnsigned(INT2 value) {
            return static_cast<UINT2>(value);
        }
    };

    template <typename INT2, typename UINT2>
    struct ToUnsignedT<INT2, UINT2, true> {
        static UINT2 ToUnsigned(INT2 value) {
            return static_cast<UINT2>(value < 0 ? -value : value);
        }
    };

    // This set of templates is very similar to the above templates, but
    // for testing whether an integer is negative.
    template <typename INT2, bool NEG2>
    struct TestNegT {};
    template <typename INT2>
    struct TestNegT<INT2, false> {
        static bool TestNeg(INT2 value) {
            // value is unsigned, and can never be negative.
            return false;
        }
    };
    template <typename INT2>
    struct TestNegT<INT2, true> {
        static bool TestNeg(INT2 value) {
            return value < 0;
        }
    };

    static STR IntToString(INT value) {
        // log10(2) ~= 0.3 bytes needed per bit or per byte log10(2**8) ~= 2.4.
        // So round up to allocate 3 output characters per byte, plus 1 for '-'.
        const int kOutputBufSize = 3 * sizeof(INT) + 1;

        // Allocate the whole string right away, we will right back to front,
        // and then return the substr of what we ended up using.
        STR outbuf(kOutputBufSize, 0);

        bool is_neg = TestNegT<INT, NEG>::TestNeg(value);
        // Even though is_neg will never be true when INT is parameterized as
        // unsigned, even the presence of the unary operation causes a warning.
        UINT res = ToUnsignedT<INT, UINT, NEG>::ToUnsigned(value);

        typename STR::iterator it(outbuf.end());
        do {
            --it;
            DCHECK(it != outbuf.begin());
            *it = static_cast<typename STR::value_type>((res % 10) + '0');
            res /= 10;
        } while (res != 0);
        if (is_neg) {
            --it;
            DCHECK(it != outbuf.begin());
            *it = static_cast<typename STR::value_type>('-');
        }
        return STR(it, outbuf.end());
    }
};

// Utility to convert a character to a digit in a given base
template<typename CHAR, int BASE, bool BASE_LTE_10> class BaseCharToDigit {
};

// Faster specialization for bases <= 10
template<typename CHAR, int BASE> class BaseCharToDigit<CHAR, BASE, true> {
public:
    static bool Convert(CHAR c, uint8* digit) {
        if (c >= '0' && c < '0' + BASE) {
            *digit = c - '0';
            return true;
        }
        return false;
    }
};

// Specialization for bases where 10 < base <= 36
template<typename CHAR, int BASE> class BaseCharToDigit<CHAR, BASE, false> {
public:
    static bool Convert(CHAR c, uint8* digit) {
        if (c >= '0' && c <= '9') {
            *digit = c - '0';
        } else if (c >= 'a' && c < 'a' + BASE - 10) {
            *digit = c - 'a' + 10;
        } else if (c >= 'A' && c < 'A' + BASE - 10) {
            *digit = c - 'A' + 10;
        } else {
            return false;
        }
        return true;
    }
};

template<int BASE, typename CHAR> bool CharToDigit(CHAR c, uint8* digit) {
    return BaseCharToDigit<CHAR, BASE, BASE <= 10>::Convert(c, digit);
}

// There is an IsWhitespace for wchars defined in string_util.h, but it is
// locale independent, whereas the functions we are replacing were
// locale-dependent. TBD what is desired, but for the moment let's not introduce
// a change in behaviour.
template<typename CHAR> class WhitespaceHelper {
};

template<> class WhitespaceHelper<char> {
public:
    static bool Invoke(char c) {
        return 0 != isspace(static_cast<unsigned char>(c));
    }
};

template<typename CHAR> bool LocalIsWhitespace(CHAR c) {
    return WhitespaceHelper<CHAR>::Invoke(c);
}

// IteratorRangeToNumberTraits should provide:
//  - a typedef for iterator_type, the iterator type used as input.
//  - a typedef for value_type, the target numeric type.
//  - static functions min, max (returning the minimum and maximum permitted
//    values)
//  - constant kBase, the base in which to interpret the input
template<typename IteratorRangeToNumberTraits>
class IteratorRangeToNumber {
public:
    typedef IteratorRangeToNumberTraits traits;
    typedef typename traits::iterator_type const_iterator;
    typedef typename traits::value_type value_type;

    // Generalized iterator-range-to-number conversion.
    //
    static bool Invoke(const_iterator begin,
                       const_iterator end,
                       value_type* output) {
        bool valid = true;

        while (begin != end && LocalIsWhitespace(*begin)) {
            valid = false;
            ++begin;
        }

        if (begin != end && *begin == '-') {
            if (!std::numeric_limits<value_type>::is_signed) {
                valid = false;
            } else if (!Negative::Invoke(begin + 1, end, output)) {
                valid = false;
            }
        } else {
            if (begin != end && *begin == '+') {
                ++begin;
            }
            if (!Positive::Invoke(begin, end, output)) {
                valid = false;
            }
        }

        return valid;
    }

private:
    // Sign provides:
    // - a static function, CheckBounds, that determines whether the next digit
    //   causes an overflow/underflow
    // - a static function, Increment, that appends the next digit appropriately
    //   according to the sign of the number being parsed.
    template<typename Sign>
    class Base {
    public:
        static bool Invoke(const_iterator begin, const_iterator end,
                           typename traits::value_type* output) {
            *output = 0;

            if (begin == end) {
                return false;
            }

            // Note: no performance difference was found when using template
            // specialization to remove this check in bases other than 16
            if (traits::kBase == 16 && end - begin > 2 && *begin == '0' &&
                (*(begin + 1) == 'x' || *(begin + 1) == 'X')) {
                begin += 2;
            }

            for (const_iterator current = begin; current != end; ++current) {
                uint8 new_digit = 0;

                if (!CharToDigit<traits::kBase>(*current, &new_digit)) {
                    return false;
                }

                if (current != begin) {
                    if (!Sign::CheckBounds(output, new_digit)) {
                        return false;
                    }
                    *output *= traits::kBase;
                }

                Sign::Increment(new_digit, output);
            }
            return true;
        }
    };

    class Positive : public Base<Positive> {
    public:
        static bool CheckBounds(value_type* output, uint8 new_digit) {
            if ((*output >
                 static_cast<value_type>(traits::max() / traits::kBase)) ||
                ((*output ==
                 static_cast<value_type>(traits::max() / traits::kBase) &&
                  new_digit > traits::max() % traits::kBase))) {
                *output = traits::max();
                return false;
            }
            return true;
        }
        static void Increment(uint8 increment, value_type* output) {
            *output += increment;
        }
    };

    class Negative : public Base<Negative> {
    public:
        static bool CheckBounds(value_type* output, uint8 new_digit) {
            if (*output < traits::min() / traits::kBase ||
                (*output == traits::min() / traits::kBase &&
                 new_digit > 0 - traits::min() % traits::kBase)) {
                *output = traits::min();
                return false;
            }
            return true;
        }
        static void Increment(uint8 increment, value_type* output) {
            *output -= increment;
        }
    };
};

template<typename ITERATOR, typename VALUE, int BASE>
class BaseIteratorRangeToNumberTraits {
public:
    typedef ITERATOR iterator_type;
    typedef VALUE value_type;
    static value_type min() {
        return std::numeric_limits<value_type>::min();
    }
    static value_type max() {
        return std::numeric_limits<value_type>::max();
    }
    static const int kBase = BASE;
};

template<typename ITERATOR>
class BaseHexIteratorRangeToIntTraits
        : public BaseIteratorRangeToNumberTraits<ITERATOR, int, 16> {
};

template<typename ITERATOR>
class BaseHexIteratorRangeToUIntTraits
        : public BaseIteratorRangeToNumberTraits<ITERATOR, uint32, 16> {
};

template<typename ITERATOR>
class BaseHexIteratorRangeToInt64Traits
        : public BaseIteratorRangeToNumberTraits<ITERATOR, int64, 16> {
};

template<typename ITERATOR>
class BaseHexIteratorRangeToUInt64Traits
        : public BaseIteratorRangeToNumberTraits<ITERATOR, uint64, 16> {
};

typedef BaseHexIteratorRangeToIntTraits<std::string::const_iterator>
HexIteratorRangeToIntTraits;

typedef BaseHexIteratorRangeToUIntTraits<std::string::const_iterator>
HexIteratorRangeToUIntTraits;

typedef BaseHexIteratorRangeToInt64Traits<std::string::const_iterator>
HexIteratorRangeToInt64Traits;

typedef BaseHexIteratorRangeToUInt64Traits<std::string::const_iterator>
HexIteratorRangeToUInt64Traits;

template<typename STR>
bool HexStringToBytesT(const STR& input, std::vector<uint8>* output) {
    DCHECK_EQ(output->size(), 0u);
    size_t count = input.size();
    if (count == 0 || (count % 2) != 0)
        return false;
    for (uintptr_t i = 0; i < count / 2; ++i) {
        uint8 msb = 0;  // most significant 4 bits
        uint8 lsb = 0;  // least significant 4 bits
        if (!CharToDigit<16>(input[i * 2], &msb) ||
            !CharToDigit<16>(input[i * 2 + 1], &lsb))
            return false;
        output->push_back((msb << 4) | lsb);
    }
    return true;
}

template <typename VALUE, int BASE>
class StringToNumberTraits
        : public BaseIteratorRangeToNumberTraits<std::string::const_iterator,
                                                 VALUE,
                                                 BASE> {
};

template <typename VALUE>
bool StringToIntImpl(const std::string& input, VALUE* output) {
    return IteratorRangeToNumber<StringToNumberTraits<VALUE, 10> >::Invoke(
        input.begin(), input.end(), output);
}

}  // namespace

std::string IntToString(int value)
{
    return IntToStringT<std::string, int, unsigned int, true>::
            IntToString(value);
}
std::string UintToString(unsigned value)
{
    return IntToStringT<std::string, unsigned int, unsigned int, false>::
            IntToString(value);
}
std::string Int64ToString(int64 value)
{
    return IntToStringT<std::string, int64, uint64, true>::
            IntToString(value);
}
std::string Uint64ToString(uint64 value)
{
    return IntToStringT<std::string, uint64, uint64, false>::
            IntToString(value);
}
std::string SizeTToString(size_t value)
{
    return IntToStringT<std::string, size_t, size_t, false>::
            IntToString(value);
}
std::string DoubleToString(double value)
{
    return "TODO(gene.ge)";
}

bool StringToInt(const std::string &str, int *value)
{
    return StringToIntImpl(str, value);
}
bool StringToUInt(const std::string &str, unsigned *value)
{
    return StringToIntImpl(str, value);
}
bool StringToInt64(const std::string &str, int64 *value)
{
    return StringToIntImpl(str, value);
}
bool StringToUint64(const std::string &str, uint64 *value)
{
    return StringToIntImpl(str, value);
}
bool StringToSizeT(const std::string &str, size_t *value)
{
    return StringToIntImpl(str, value);
}
bool StringToDouble(const std::string &str, double *value)
{
    // TODO(gene.ge):float
    return true;
}

std::string HexEncode(const void *bytes, size_t size)
{
    static const char kHexChars[] = "0123456789ABCDEF";

    // Each input byte creates two output hex characters.
    std::string ret(size * 2, '\0');

    for (size_t i = 0; i < size; ++i) {
        char b = reinterpret_cast<const char*>(bytes)[i];
        ret[(i * 2)] = kHexChars[(b >> 4) & 0xf];
        ret[(i * 2) + 1] = kHexChars[b & 0xf];
    }
    return ret;
}
bool HexStringToInt(const std::string &str, int *value)
{
    return IteratorRangeToNumber<HexIteratorRangeToIntTraits>::Invoke (
        str.begin(), str.end(), value);
}
bool HexStringToUint(const std::string &str, unsigned *value)
{
    return IteratorRangeToNumber<HexIteratorRangeToUIntTraits>::Invoke (
        str.begin(), str.end(), value);
}
bool HexStringToInt64(const std::string &str, int64 *value)
{
    return IteratorRangeToNumber<HexIteratorRangeToInt64Traits>::Invoke (
        str.begin(), str.end(), value);
}
bool HexStringToUint64(const std::string &str, uint64 *value)
{
    return IteratorRangeToNumber<HexIteratorRangeToUInt64Traits>::Invoke (
        str.begin(), str.end(), value);
}

}  // namespace base
