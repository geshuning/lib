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

#ifndef BASE_FLAGS_H_
#define BASE_FLAGS_H_

#include <string>
#include "base/basictypes.h"
// #include "basictypes.h"

// Declare and define global flags.
// exp: DECLARE_int32(pkt_count, 0, "count the total packet number")
//      will create a variable with type int32, name FLAGS_pkt_count,
//      initialized with 0
//
#define DECLARE_VARIABLE(type, shorttype, name, tn) \
    namespace fL##shorttype {                       \
        extern type FLAGS_##name;                   \
    }                                               \
    using fL##shorttype::FLAGS_##name;
#define DEFINE_VARIABLE(type, shorttype, name, value, meaning, tn)      \
    namespace fL##shorttype {                                           \
        type FLAGS_##name(value);                                       \
        char FLAGS_no##name;                                            \
    }                                                                   \
    using fL##shorttype::FLAGS_##name;

#define DECLARE_bool(name)                      \
    DECLARE_VARIABLE(bool, B, name, bool)
#define DEFINE_bool(name, value, meaning)               \
    DEFINE_VARIABLE(bool, B, name, value, meaning, bool)

#define DECLARE_int32(name)                     \
    DECLARE_VARIABLE(int32, I, name, int32)
#define DEFINE_int32(name, value, meaning)                  \
    DEFINE_VARIABLE(int32, I, name, value, meaning, int32)

#define DECLARE_string(name)                    \
    namespace fLS {                             \
        exterm std::string& FLAGS_##name;       \
    }                                           \
    using fLS::FLAGS_##name;
#define DEFINE_string(name, value, meaning)             \
    namespace fLS {                                     \
        std::string FLAGS_##name##_buf(value);          \
        std::string& FLAGS_##name = FALGS_##name##_buf; \
        char FLAGS_no##name;                            \
    }                                                   \
    using fLS::FLAGS_##name;

#endif  // BASE_FLAGS_H_
