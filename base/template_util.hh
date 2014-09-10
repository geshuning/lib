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

#ifndef BASE_TEMPLATE_UTIL_HH_
#define BASE_TEMPLATE_UTIL_HH_

#include <cstddef>              // For size_t

namespace base {
template <typename T, T v>
struct integral_constant {
    static const T value = v;
    typedef T value_type;
    typedef integral_constant<T, v> type;
};

template <typename T, T v>
const T integral_constant<T, v>::value;

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

// private inherit
template <typename T> struct is_pointer : false_type {};
template <typename T> struct is_pointer<T*> : true_type {};

template <typename T, typename U> struct is_same : public false_type {};
template <typename T> struct is_same<T, T> : true_type {};

template <typename> struct is_array : public false_type {};
template <typename T, size_t n> struct is_array<T[n]> : public true_type {};
template <typename T> struct is_array<T[]> : public true_type {};

template <typename T> struct is_non_const_reference : false_type {};
template <typename T> struct is_non_const_reference<T&> : true_type {};
template <typename T> struct is_non_const_reference<const T&> : false_type {};

template <typename T> struct is_void : false_type {};
template <> struct is_void<void> : true_type {};

namespace internal {
typedef char YesType;
struct NoType {
    YesType dummy[2];
};

struct ConvertHelper {
    template <typename To>
    static YesType Test(To);

    template <typename To>
    static NoType Test(To);

    template <typename From>
    static From& Create();
};

// Used to determine if a type is a struct/union/class. Inspired by Boost's
// is_class type_trait implementation.
struct IsClassHelper {
    template <typename C>
    static YesType Test(void(C::*)(void));

    template <typename C>
    static NoType Test(...);
};
}  // namespace internal

template <typename From, typename To>
struct is_convertible
    : integral_constant<bool,
    sizeof(internal::ConvertHelper::Test<To>(
               internal::ConvertHelper::Create<From>()))
    == sizeof(internal::YesType)> {
};

template <typename T>
struct is_class
    : integral_constant<bool,
    sizeof(internal::IsClassHelper::Test<T>(0)) ==
    sizeof(internal::YesType)> {
};

}  // namespace base
#endif  // BASE_TEMPLATE_UTIL_HH_
