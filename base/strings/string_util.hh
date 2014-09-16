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

#ifndef BASE_STRINGS_STRING_UTIL_HH_
#define BASE_STRINGS_STRING_UTIL_HH_

int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t count);

int vsnprintf(char *buffer, size_t size, const char *format, va_list ap);
inline int snprintf(char *buffer, size_t size, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int result = vsnprintf(buffer, size, format, arguments);
    va_end(arguments);
    return result;
}

// ASCII-specific tolower.  The standard library's tolower is locale sensitive,
// so we don't want to use it here.
template <class Char> inline Char ToLowerASCII(Char c) {
    return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

// ASCII-specific toupper.  The standard library's toupper is locale sensitive,
// so we don't want to use it here.
template <class Char> inline Char ToUpperASCII(Char c) {
    return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
}

template<typename Char> struct CaseInsensitiveCompareASCII {
public:
    bool operator()(Char x, Char y) const {
        return ToLowerASCII(x) == ToLowerASCII(y);
    }
};

extern const char kWhitespaceASCII[];


#endif  // BASE_STRINGS_STRING_UTIL_HH_
