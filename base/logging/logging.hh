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

#ifndef BASE_LOGGING_LOGGING_HH_
#define BASE_LOGGING_LOGGING_HH_

#include <string>
#include <vector>
#include <iostream>
#include <iosfwd>
#include <sstream>
#include <list>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sstream>
#include <iomanip>
#include <map>

#include "base/basictypes.hh"
#include "base/compiler_specific.hh"
#include "base/flags.hh"
#include "base/logging/syslog.hh"
#include "base/synchronization/lock.hh"
#include "base/time/time.hh"

typedef base::AutoLock MutexLock;
typedef base::Lock Mutex;

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

// Log severity enum
#define LOG_SEVERITYS                             \
    LOG_SEVERITY(FATAL, SYSLOG_CRITICAL)          \
    LOG_SEVERITY(ERR, SYSLOG_ERROR)               \
    LOG_SEVERITY(WARNING, SYSLOG_WARNING)         \
    LOG_SEVERITY(INFO, SYSLOG_INFORMATIONAL)      \
    LOG_SEVERITY(DEBUG, SYSLOG_DEBUG)

enum LogSeverity
{
#define LOG_SEVERITY(NAME, SEVERITY) kLS_##NAME,
    LOG_SEVERITYS
#undef LOG_SEVERITY
    kLS_MAX
};

// base_logging
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

// Log Destination
enum Log_Destination {
    kLOG_DST_FILE,
    kLOG_DST_STDERR,
    kLOG_DST_SYSLOG,
    kLOG_DST_SOCK,
    kLOG_DST_MAX
};

class LogDestination {
public:
    LogDestination() : log_fd_(-1), type(kLOG_DST_MAX)
    {
    }
    ~LogDestination()
    {
    }
    virtual void Log(LogSeverity severity, time_t timestamp,
                     const char* message, size_t len) = 0;
public:
    enum Log_Destination type;
    int log_fd_;
private:
    DISALLOW_COPY_AND_ASSIGN(LogDestination);
};

class LogDestinationToFile : public LogDestination {
public:
    LogDestinationToFile(std::string name);
    ~LogDestinationToFile();
    void FlushUnLocked();
    virtual void Log(LogSeverity severity, time_t timestamp,
                     const char* message, size_t len);
private:
  Mutex lock_;
  std::string base_filename_;
  FILE* file_;
  uint32_t file_length_;
  uint32_t bytes_since_flush_;
  uint32_t bytes_max_flush_;
  int64_t next_flush_time_;
  DISALLOW_COPY_AND_ASSIGN(LogDestinationToFile);
};

// module
class LogModule {
public:
    const std::string name;
    LogSeverity min_severity;
    bool vlog_on;
    uint32_t n_bytes;
    uint32_t max_bytes;
    LogDestination *severity_dsts[kLS_MAX][kLOG_DST_MAX];
    LogModule(const std::string m_name);
    void AddLogDestination(LogDestination *dst, LogSeverity severity);
private:
    DISALLOW_COPY_AND_ASSIGN(LogModule);
};

#define INFO_LOG_TO_FILE(name)                                      \
    do {                                                            \
        LogDestinationToFile *dst = new LogDestinationToFile(name); \
        THIS_MODULE->AddLogDestination(dst, kLS_INFO);              \
    } while(0)

class LogModule;
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
    /*
      LogMessage(const char* file, int line, const CheckOpString& result);*/
    LogMessage(){}
    LogMessage(const char* file, int line, const CheckOpString& result){}
    LogMessage(const LogModule *module, const char* file, int line,
               LogSeverity severity);
    ~LogMessage();
    void Flush();
    static const size_t kMaxLogMessageLen;
    void SendToLog();
    void Fail();
    std::ostream& stream();
    struct LogMessageData;

private:
    void Init(const LogModule* module, const char* file, int line,
              LogSeverity severity);
private:
    LogMessageData* allocated_;
    LogMessageData* data_;
    LogModule *module_;
    friend class LogModule;

private:
    DISALLOW_COPY_AND_ASSIGN(LogMessage);
};

// LogMesssageFatal mainly used in CHECK macro,
// When CHECK failed, it will log a FATAL ERROR
class LogMessageFatal : public LogMessage {
  public:
    /*
  LogMessageFatal(const char* file, int line);
  LogMessageFatal(const char* file, int line, const CheckOpString& result);
  ~LogMessageFatal();
  */
    LogMessageFatal(const char* file, int line){}
  LogMessageFatal(const char* file, int line, const CheckOpString& result){}
  ~LogMessageFatal(){}
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


#define LOG_DEFINE_MODULE(MODULE)               \
    static LogModule log_module_##MODULE(#MODULE)

#define LOG_DEFINE_THIS_MODULE(MODULE)                          \
    LOG_DEFINE_MODULE(MODULE);                                  \
    static LogModule *const THIS_MODULE = &log_module_##MODULE

#define LOG_INFO() LogMessage(THIS_MODULE, \
                              __FILE__, __LINE__, kLS_INFO).stream()
#define LOG_WARN() LogMessage(THIS_MODULE, \
                              __FILE__, __LINE__, kLS_WARNING).stream()
#define LOG_ERR() LogMessage(THIS_MODULE, \
                             __FILE__, __LINE__, kLS_ERROR).stream()
#define LOG_FATAL() LogMessage(THIS_MODULE, \
                               __FILE__, __LINE__, kLS_FATAL).stream()

#define LOG_INFO_RL(RL)
#define LOG_WARN_RL(RL)
#define LOG_ERR_RL(RL)
#define LOG_FATAL_RL(RL)
#if 0
#define COMPACT_LOG_INFO LogMessage(__FILE__, __LINE__)
#define COMPACT_LOG_WARNING LogMessage(__FILE__, __LINE__, kLS_WARNING)
#define COMPACT_LOG_ERROR LogMessage(__FILE__, __LINE__, kLS_ERROR)
#define COMPACT_LOG_FATAL LogMessage(__FILE__, __LINE__, kLS_FATAL)
#define LOG(severity) COMPACT_LOG_##severity.stream()
#endif
#define LOG(severity) LogMessage().stream()

#define LOG_IF(severity, condition) \
  !(condition) ? (void) 0 : LogMessageVoidify() & LOG(severity)

// TODO(geshuning):VLOG
#define VLOG(verboselevel) LOG_IF(INFO, false)
#define DLOG(severity) \
  true ? (void) 0 : LogMessageVoidify() & LOG(severity)
#define DVLOG(verboselevel) VLOG(verboselevel)

namespace base {

}  // namespace base
#include "base/logging/check.hh"
#endif  // BASE_LOGGING_LOGGING_HH_
