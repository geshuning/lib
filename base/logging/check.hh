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

#ifndef BASE_LOGGING_CHECK_HH_
#define BASE_LOGGING_CHECK_HH_

#include <string>
#include <vector>
#include <iostream>
#include <iosfwd>
#include <sstream>
#include "base/basictypes.hh"

// CHECK MACROs
// Function is overloaded for integral types to allow static const
// integrals declared in classes and not defined to be used as arguments to
// CHECK* macros. It's not encouraged though.
template <class T>
inline const T&       GetReferenceableValue(const T&           t) { return t; }
inline char           GetReferenceableValue(char               t) { return t; }
inline unsigned char  GetReferenceableValue(unsigned char      t) { return t; }
inline signed char    GetReferenceableValue(signed char        t) { return t; }
inline short          GetReferenceableValue(short              t) { return t; }
inline unsigned short GetReferenceableValue(unsigned short     t) { return t; }
inline int            GetReferenceableValue(int                t) { return t; }
inline unsigned int   GetReferenceableValue(unsigned int       t) { return t; }
inline long           GetReferenceableValue(long               t) { return t; }
inline unsigned long  GetReferenceableValue(unsigned long      t) { return t; }
inline long long      GetReferenceableValue(long long          t) { return t; }
inline unsigned long long GetReferenceableValue(unsigned long long t) {
  return t;
}

// This is a dummy class to define the following operator.
struct DummyClassToDefineOperator {};

// Define global operator<< to declare using ::operator<<.
// This declaration will allow use to use CHECK macros for user
// defined classes which have operator<< (e.g., stl_logging.h).
inline std::ostream& operator<<(
  std::ostream& out, const DummyClassToDefineOperator&) {
  return out;
}

template <typename T>
inline void MakeCheckOpValueString(std::ostream* os, const T& v) {
  (*os) << v;
}

// Overrides for char types provide readable valuse for unprintable
// characters.
template <>
inline void MakeCheckOpValueString(std::ostream* os, const char& v) {
  if (v >= 32 && v <= 126) {
    (*os) << "'" << v << "'";
  } else {
    (*os) << "char value " << (short)v;
  }
}

template <>
inline void MakeCheckOpValueString(std::ostream* os, const signed char& v) {
  if (v >= 32 && v <= 126) {
    (*os) << "'" << v << "'";
  } else {
    (*os) << "signed char value " << (short)v;
  }
}

template <>
inline void MakeCheckOpValueString(std::ostream* os, const unsigned char& v) {
  if (v >= 32 && v <= 126) {
    (*os) << "'" << v << "'";
  } else {
    (*os) << "unsigned char value " << (unsigned short)v;
  }
}

class CheckOpMessageBuilder {
  public:
  explicit CheckOpMessageBuilder(const char* exprtext)
    :stream_(new std::ostringstream) {
    *stream_ << exprtext << " (";
  }
  ~CheckOpMessageBuilder() {
    delete stream_;
  }
  std::ostream* ForVar1() {
    return stream_;
  }
  std::ostream* ForVar2() {
    *stream_ << " vs. ";
    return stream_;
  }
  std::string* NewString() {
    *stream_ << ")";
    return new std::string(stream_->str());
  }
  private:
  std::ostringstream *stream_;
};

template <typename T1, typename T2>
std::string* MakeCheckOpString(const T1& v1, const T2& v2, const char* exprtext) {
  CheckOpMessageBuilder comb(exprtext);
  MakeCheckOpValueString(comb.ForVar1(), v1);
  MakeCheckOpValueString(comb.ForVar2(), v2);
  return comb.NewString();
}

#define DEFINE_CHECK_OP_IMPL(name, op) \
  template <typename T1, typename T2> \
    inline std::string* name##Impl(const T1& v1, const T2& v2, \
                                   const char* exprtext) { \
    if (PREDICT_TRUE(v1 op v2)) return NULL; \
    else return MakeCheckOpString(v1, v2, exprtext); \
  } \
  inline std::string* name##Impl(int v1, int v2, const char* exprtext) { \
    return name##Impl<int, int>(v1, v2, exprtext);                       \
  }

DEFINE_CHECK_OP_IMPL(Check_EQ, ==)
DEFINE_CHECK_OP_IMPL(Check_NE, !=)
DEFINE_CHECK_OP_IMPL(Check_LE, <=)
DEFINE_CHECK_OP_IMPL(Check_LT, < )
DEFINE_CHECK_OP_IMPL(Check_GE, >=)
DEFINE_CHECK_OP_IMPL(Check_GT, > )
#undef DEFINE_CHECK_OP_IMPL

#define CHECK_OP_LOG(name, op, val1, val2, log) \
  while (std::string* _result = \
         Check##name##Impl(GetReferenceableValue(val1), \
                           GetReferenceableValue(val2), \
                           #val1 " " #op " " #val2)) \
    log(__FILE__, __LINE__, _result).stream()

#if STRIP_LOG <= 3
#define CHECK_OP(name, op, val1, val2) \
  CHECK_OP_LOG(name, op, val1, val2, LogMessageFatal)
#else
#define CHECK_OP(name, op, val1, val2) \
  CHECK_OP_LOG(name, op, val1, val2, NullStreamFatal)
#endif

// Equality/Inequality checks - compare two values, and log a FATAL message
// including the two values when the result is not as expected.  The values
// must have operator<<(ostream, ...) defined.
//
// You may append to the error message like so:
//   CHECK_NE(1, 2) << ": The world must be ending!";
//
// We are very careful to ensure that each argument is evaluated exactly
// once, and that anything which is legal to pass as a function argument is
// legal here.  In particular, the arguments may be temporary expressions
// which will end up being destroyed at the end of the apparent statement,
// for example:
//   CHECK_EQ(string("abc")[1], 'b');
//
// WARNING: These don't compile correctly if one of the arguments is a pointer
// and the other is NULL. To work around this, simply static_cast NULL to the
// type of the desired pointer.
#define CHECK_EQ(val1, val2) CHECK_OP(_EQ, ==, val1, val2)
#define CHECK_NE(val1, val2) CHECK_OP(_NE, !=, val1, val2)
#define CHECK_LE(val1, val2) CHECK_OP(_LE, <=, val1, val2)
#define CHECK_LT(val1, val2) CHECK_OP(_LT, < , val1, val2)
#define CHECK_GE(val1, val2) CHECK_OP(_GE, >=, val1, val2)
#define CHECK_GT(val1, val2) CHECK_OP(_GT, > , val1, val2)

// Check that the input in non NULL. this very useful in constructor
// initializer lists.
template <typename T>
T* CheckNotNull(const char* file, int line, const char* names, T* t) {
  if (t == NULL) {
    // There is no need to cleaning up the third parameter,
    // Because we will call log(FATAL)
    LogMessageFatal(file, line, new std::string(names));
  }
  return t;
}
#define CHECK_NOTNULL(val) \
  CheckNotNull(__FILE__, __LINE__, "'", #val "' Must be non NULL", (val))


// Helper functions for string comparisons.
// To avoid bloat, the definitions are in logging.cc.
#define DECLARE_CHECK_STROP_IMPL(func, expected) \
  std::string* Check##func##expected##Impl( \
    const char* s1, const char* s2, const char* names);
DECLARE_CHECK_STROP_IMPL(strcmp, true)
DECLARE_CHECK_STROP_IMPL(strcmp, false)
DECLARE_CHECK_STROP_IMPL(strcasecmp, true)
DECLARE_CHECK_STROP_IMPL(strcasecmp, false)
#undef DECLARE_CHECK_STROP_IMPL

// Helper macro for string comparisons.
// Don't use this macro directly in your code, use CHECK_STREQ et al below.
#define CHECK_STROP(func, op, expected, s1, s2) \
  while (CheckOpString _result = \
                   Check##func##expected##Impl((s1), (s2), \
                                                       #s1 " " #op " " #s2)) \
    LOG(FATAL) << *_result.str_


// String (char*) equality/inequality checks.
// CASE versions are case-insensitive.
//
// Note that "s1" and "s2" may be temporary strings which are destroyed
// by the compiler at the end of the current "full expression"
// (e.g. CHECK_STREQ(Foo().c_str(), Bar().c_str())).

#define CHECK_STREQ(s1, s2) CHECK_STROP(strcmp, ==, true, s1, s2)
#define CHECK_STRNE(s1, s2) CHECK_STROP(strcmp, !=, false, s1, s2)
#define CHECK_STRCASEEQ(s1, s2) CHECK_STROP(strcasecmp, ==, true, s1, s2)
#define CHECK_STRCASENE(s1, s2) CHECK_STROP(strcasecmp, !=, false, s1, s2)

#define CHECK_INDEX(I,A) CHECK(I < (sizeof(A)/sizeof(A[0])))
#define CHECK_BOUND(B,A) CHECK(B <= (sizeof(A)/sizeof(A[0])))

#define CHECK_DOUBLE_EQ(val1, val2) \
  do { \
  CHECK_LE((val1), (val2)+0.000000000000001L); \
  CHECK_GE((val1), (val2)-0.000000000000001L); \
  } while (0)

#define CHECK_NEAR(val1, val2, margin) \
  do { \
  CHECK_LE((val1), (val2)+(margin)); \
  CHECK_GE((val1), (val2)-(margin)); \
  } while (0)

// CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by NDEBUG, so the check will be executed regardless of
// compilation mode.  Therefore, it is safe to do things like:
//    CHECK(fp->Write(x) == 4)
#define CHECK(condition)  \
  LOG_IF(FATAL, PREDICT_BRANCH_NOT_TAKEN(!(condition))) \
  << "Check failed: " #condition " "


// Synonyms for CHECK_* that are used in some unittests.
/*
#define EXPECT_EQ(val1, val2) CHECK_EQ(val1, val2)
#define EXPECT_NE(val1, val2) CHECK_NE(val1, val2)
#define EXPECT_LE(val1, val2) CHECK_LE(val1, val2)
#define EXPECT_LT(val1, val2) CHECK_LT(val1, val2)
#define EXPECT_GE(val1, val2) CHECK_GE(val1, val2)
#define EXPECT_GT(val1, val2) CHECK_GT(val1, val2)
#define ASSERT_EQ(val1, val2) EXPECT_EQ(val1, val2)
#define ASSERT_NE(val1, val2) EXPECT_NE(val1, val2)
#define ASSERT_LE(val1, val2) EXPECT_LE(val1, val2)
#define ASSERT_LT(val1, val2) EXPECT_LT(val1, val2)
#define ASSERT_GE(val1, val2) EXPECT_GE(val1, val2)
#define ASSERT_GT(val1, val2) EXPECT_GT(val1, val2)
// As are these variants.
#define EXPECT_TRUE(cond)     CHECK(cond)
#define EXPECT_FALSE(cond)    CHECK(!(cond))
#define EXPECT_STREQ(a, b)    CHECK(strcmp(a, b) == 0)
#define ASSERT_TRUE(cond)     EXPECT_TRUE(cond)
#define ASSERT_FALSE(cond)    EXPECT_FALSE(cond)
#define ASSERT_STREQ(a, b)    EXPECT_STREQ(a, b)
*/

// Synonyms for CHECK_* that are used in DEBUG
#define DCHECK(condition) CHECK(condition)
#define DCHECK_EQ(val1, val2) CHECK_EQ(val1, val2)
#define DCHECK_NE(val1, val2) CHECK_NE(val1, val2)
#define DCHECK_LE(val1, val2) CHECK_LE(val1, val2)
#define DCHECK_LT(val1, val2) CHECK_LT(val1, val2)
#define DCHECK_GE(val1, val2) CHECK_GE(val1, val2)
#define DCHECK_GT(val1, val2) CHECK_GT(val1, val2)
// As are these variants.
#define DCHECK_TRUE(cond)     CHECK(cond)
#define DCHECK_FALSE(cond)    CHECK(!(cond))
#define DCHECK_STREQ(a, b)    CHECK(strcmp(a, b) == 0)

// CHECK MACROs end

#endif  // BASE_LOGGING_CHECK_HH_
