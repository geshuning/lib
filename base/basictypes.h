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

#ifndef BASE_BASICTYPES_H_
#define BASE_BASICTYPES_H_
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

typedef signed char schar;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// NOTE: unsigned types are DANGEROUS in loops and other arithmetical
// places.  Use the signed types unless your variable represents a bit
// pattern (eg a hash value) or you really need the extra bit.  Do NOT
// use 'unsigned' to express "this value should always be positive";
// use assertions for this.
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

const uint16 kuint16max = (static_cast<uint16>(0xFFFF));
const uint32 kuint32max = (static_cast<uint32>(0xFFFFFFFF));
const uint64 kuint64max = (((static_cast<uint64>(kuint32max)) << 32)
                           | kuint32max);

const  int8  kint8max   = (static_cast<int8>(0x7F));
const  int16 kint16max  = (static_cast<int16>(0x7FFF));
const  int32 kint32max  = (static_cast<int32>(0x7FFFFFFF));
const  int64 kint64max  =  (((static_cast<int64>(kint32max)) << 32)
                           | kuint32max);

const  int8  kint8min   = (static_cast<int8>(0x80));
const  int16 kint16min  = (static_cast<int16>(0x8000));
const  int32 kint32min  = (static_cast<int32>(0x80000000));
const  int64 kint64min  = (((static_cast<int64>(kint32min)) << 32) | 0);

// If inttypes.h included already, do nothing
#ifndef PRIx64
#define PRIx64 "llx"
#endif
#ifndef SCNx64
#define SCNx64 "llx"
#endif
#ifndef PRId64
#define PRId64 "lld"
#endif
#ifndef SCNd64
#define SCNd64 "lld"
#endif
#ifndef PRIu64
#define PRIu64 "llu"
#endif
#ifndef PRIxPTR
#define PRIxPTR "lx"
#endif

// Put the following "DISALLOW_XXX in private"
#define DISALLOW_COPY(TypeName)                 \
    TypeName(const TypeName&)

#define DISALLOW_ASSIGN(TypeName)               \
    void operator=(const TypeName&)
// A macro to disallow the evil copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_CONPY_AND_ASSIGN(TypeName)     \
    TypeName(const TypeName&);                  \
    void operator=(const TypeName&)
// Declarations for a class that wants to prevent anyone from instantiating it.
// This is especially useful for classes containing only static methods
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
    TypeName();                                  \
    DISALLOW_COPY_AND_ASSIGN(TypeName)

// The COMPILE_ASSERT macro can be used to verify that a compile time
// expression is true. For example, you could use it to verify the
// size of a static array:
//
//   COMPILE_ASSERT(sizeof(num_content_type_names) == sizeof(int),
//                  content_type_names_incorrect_size);
//
// or to make sure a struct is smaller than a certain size:
//
//   COMPILE_ASSERT(sizeof(foo) < 128, foo_too_large);
//
// The second argument to the macro is the name of the variable. If
// the expression is false, most compilers will issue a warning/error
// containing the name of the variable.
//
// Implementation details of COMPILE_ASSERT:
//
// - COMPILE_ASSERT works by defining an array type that has -1
//   elements (and thus is invalid) when the expression is false.
//
// - The simpler definition
//
//     #define COMPILE_ASSERT(expr, msg) typedef char msg[(expr) ? 1 : -1]
//
//   does not work, as gcc supports variable-length arrays whose sizes
//   are determined at run-time (this is gcc's extension and not part
//   of the C++ standard).  As a result, gcc fails to reject the
//   following code with the simple definition:
//
//     int foo;
//     COMPILE_ASSERT(foo, msg); // not supposed to compile as foo is
//                               // not a compile-time constant.
//
// - By using the type CompileAssert<(bool(expr))>, we ensures that
//   expr is a compile-time constant.  (Template arguments must be
//   determined at compile-time.)
//
// - The outter parentheses in CompileAssert<(bool(expr))> are necessary
//   to work around a bug in gcc 3.4.4 and 4.0.1.  If we had written
//
//     CompileAssert<bool(expr)>
//
//   instead, these compilers will refuse to compile
//
//     COMPILE_ASSERT(5 > 0, some_message);
//
//   (They seem to think the ">" in "5 > 0" marks the end of the
//   template argument list.)
//
// - The array size is (bool(expr) ? 1 : -1), instead of simply
//
//     ((expr) ? 1 : -1).
//
//   This is to avoid running into a bug in MS VC 7.1, which
//   causes ((0.0) ? 1 : -1) to incorrectly evaluate to 1.
template <bool>
struct CompileAssert {
};
#define COMPILE_ASSERT(expr, msg)                                       \
    typedef CompileAssert<static_cast<bool>(expr)> msg[static_cast<bool>(expr) \
                                                       ? 1 : -1]

// The arraysize macro returns the count of elements in an array
// The expression is a compile-time constant, and therefore can be
// used in defining new arrays.
// One caveat is that arraysize() doessn't accept array of an anonymous type
// or a type defined inside a function. In these rare cases, you have to use the
// unsafe ARRAYSIZE_UNSAFE() macro instead.

// This template function daclaration is used in defing arraysize.
// Note that the function doesn't need an implementation,
// as we only use its type.
template <typename T, size_t N>
char (&ArraySizeHelper(T(&array)[N]))[N];
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

// ARRAYSIZE_UNSAFE is a compile-time constant of type size_t
// ARRAYSIZE_UNSAFE catches a few type errors, exp, devision by zero
// If the former is divisible by the latter, perhaps a is indeed an array
// 0 otherwise, a cannot possibly be an array, and we generate a compiler error
// to prevent the code from compiling.
// Since the sizeof of bool is implementation-defined, we need to cast
// !(sizeof(a) & sizeof(*(a))) to size_t in order to
// ensure the final result has type size_t.
#define ARRAYSIZE_UNSAFE(a) \
    ((sizeof(a) / sizeof(*(a))) /                       \
     static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

#define OFFSETOF_MEMBER(strct, field)                                   \
    (reinterpret_cast<char*>(&reinterpret_cast<strct*>(16)->field)      \
     - reinterpret_cast<char*>(16))

// Used to explicitly mark the return value of a function as unused.
template <typename T>
inline void ignore_result(const T&) {
}

#endif  // BASE_BASICTYPES_H_
