// Copyright(c)2013,nsfocus
// All rights reserved.
// Author: Ge Shuning<geshuning@intra.nsfocus.com>
//

#ifndef NF_BASE_LOGGING_LOGGING_H_
#define NF_BASE_LOGGING_LOGGING_H_

#include <string>
#include <vector>
#include <iostream>
#include <iosfwd>
#include <sstream>

#include "base/basictypes.hh"
#include "base/compiler_specific.hh"
#include "base/flags.hh"

struct CheckOpString {
  CheckOpString(std::string* str) : str_(str) { }
  // No destructor: if str_ is non-null, we're about to log(FATAL)
  // So there is no need to cleaning up str_
  // Please be sure only use this in logging FATAL message
  operator bool() const {
    return PREDICT_BRANCH_NOT_TAKEN(str_ != NULL);
  }
  std::string* str_;
};

enum LogSeverity {kLS_INFO, kLS_WARNING, kLS_ERROR, kLS_FATAL, kLS_MAX};
extern const char *const LogSeverityNames[kLS_MAX];



namespace base_logging {

class LogStreamBuf : public std::streambuf {
    public:
    LogStreamBuf(char *buf, int len) {
      setp(buf, buf + len - 2);
    }
    virtual int_type overflow(int_type ch) {
      return ch;
    }
    size_t pcount() const {
      return pptr() - pbase();
    }
    char* pbase() const {
      return std::streambuf::pbase();
    }
  };
}  // namespace base_logging

class LogMessage {
  public:
  enum {kNoLogPrefix = -1};
  // LogStream begin
  class LogStream : public std::ostream {
    public:
    LogStream(char *buf, int len, int ctr)
      :std::ostream(NULL),
      streambuf_(buf, len),
      ctr_(ctr),
      self_(this) {
        rdbuf(&streambuf_);
      }
    int ctr() const {
      return ctr_;
    }
    void set_str(int ctr) {
      ctr_ = ctr;
    }
    LogStream* self() const {
      return self_;
    }
    size_t pcount() const {
      return streambuf_.pcount();
    }
    char* pbase() const {
      return streambuf_.pbase();
    }
    char* str() const {
      return pbase();
    }
 private:
    base_logging::LogStreamBuf streambuf_;
    int ctr_;
    LogStream *self_;
  };
  // LogStream end
  public:
  typedef void (LogMessage::*SendMethod)();
  LogMessage(const char* file, int line, const CheckOpString& result);
  LogMessage(const char* file, int line);
  LogMessage(const char* file, int line, LogSeverity severity);
  LogMessage(const char* module, const char* file, int line);
  LogMessage(const char* module, const char* file, int line, LogSeverity severity);
  ~LogMessage();
  void Flush();
  static const size_t kMaxLogMessageLen;
  void SendToLog();
  void Fail();
  std::ostream& stream();
  struct LogMessageData;

  private:
  void Init(const char* module, const char* file, int line,
            LogSeverity severity, void (LogMessage::*send_method)());
  private:
  // static int64 num_messages_[kLS_MAX]; // not use
  LogMessageData* allocated_;
  LogMessageData* data_;
  friend class LogDestination;

  private:
  DISALLOW_COPY_AND_ASSIGN(LogMessage);
};

// LogMesssageFatal mainly used in CHECK macro,
// When CHECK failed, it will log a FATAL ERROR
class LogMessageFatal : public LogMessage {
  public:
  LogMessageFatal(const char* file, int line);
  LogMessageFatal(const char* file, int line, const CheckOpString& result);
  ~LogMessageFatal();
};

// Allow folks to put a counter in the LOG_EVERY_X()'ed messages. This
// only works if ostream is a LogStream. If the ostream is not a
// LogStream you'll get an assert saying as much at runtime.
enum PRIVATE_Counter {COUNTER};
std::ostream& operator<<(std::ostream& os, const PRIVATE_Counter&);

// Used in macro LOG_IF()
class LogMessageVoidify {
  public:
  LogMessageVoidify() {}
  ~LogMessageVoidify() {}
  void operator&(std::ostream&) {}
};

#define COMPACT_LOG_INFO LogMessage(__FILE__, __LINE__)
#define COMPACT_LOG_WARNING LogMessage(__FILE__, __LINE__, kLS_WARNING)
#define COMPACT_LOG_ERROR LogMessage(__FILE__, __LINE__, kLS_ERROR)
#define COMPACT_LOG_FATAL LogMessage(__FILE__, __LINE__, kLS_FATAL)
#define LOG(severity) COMPACT_LOG_ ## severity.stream()

#define MODULE_COMPACT_LOG_INFO(module) \
  LogMessage(module, __FILE__, __LINE__)
#define MODULE_COMPACT_LOG_WARNING(module) \
  LogMessage(module, __FILE__, __LINE__, kLS_WARNING)
#define MODULE_COMPACT_LOG_ERROR(module) \
  LogMessage(module, __FILE__, __LINE__, kLS_ERROR)
#define MODULE_COMPACT_LOG_FATAL(module) \
  LogMessage(module, __FILE__, __LINE__, kLS_FATAL)
#define MLOG(module, severity) MODULE_COMPACT_LOG_ ## severity(module).stream()

#define LOG_IF(severity, condition) \
  !(condition) ? (void) 0 : LogMessageVoidify() & LOG(severity)

// TODO(geshuning):VLOG
#define VLOG(verboselevel) LOG_IF(INFO, false)
#define DLOG(severity) \
  true ? (void) 0 : LogMessageVoidify() & LOG(severity)
#define DVLOG(verboselevel) VLOG(verboselevel)

namespace base {

  class Logger {
    public:
    virtual ~Logger();
    virtual void Write(bool force_flush,
                       time_t timestamp,
                       const char* message,
                       int message_len) = 0;
    virtual void Flush() = 0;
    virtual uint32 LogSize() = 0;
  };

}  // namespace base
#include "base/logging/check.hh"
#endif  // NF_BASE_LOGGING_LOGGING_H_
