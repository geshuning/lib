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

#ifndef BASE_STRINGS_STRING_NUMBER_CONVERSION_HH_
#define BASE_STRINGS_STRING_NUMBER_CONVERSION_HH_

#include <string>

#include "base/basictypes.hh"

namespace base {

std::string IntToString(int value);
std::string UintToString(unsigned value);
std::string Int64ToString(int64 value);
std::string Uint64ToString(uint64 value);
std::string SizeTToString(size_t value);
std::string DoubleToString(double value);

bool StringToInt(const std::string &str, int *value);
bool StringToUInt(const std::string &str, unsigned *value);
bool StringToInt64(const std::string &str, int64 *value);
bool StringToUint64(const std::string &str, uint64 *value);
bool StringToSizeT(const std::string &str, size_t *value);
bool StringToDouble(const std::string &str, double *value);

// return string is in upper case
std::string HexEncode(const void *bytes, size_t size);
bool HexStringToInt(const std::string &str, int *value);
bool HexStringToUint(const std::string &str, unsigned *value);
bool HexStringToInt64(const std::string &str, int64 *value);
bool HexStringToUint64(const std::string &str, uint64 *value);

}      // namespace base
#endif  // BASE_STRINGS_STRING_NUMBER_CONVERSION_HH_
