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

#include "base/logging/logging.hh"

#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sstream>
#include <iomanip>
#include <map>
#include <sys/time.h>

#include "base/synchronization/lock.hh"
typedef base::AutoLock MutexLock;
typedef base::Lock Mutex;

const char* const LogSeverityNames[] = {
  "INFO", "WARNING", "ERROR", "FATAL"
};

const char* GetLogSeverityName(LogSeverity severity) {
  return LogSeverityNames[severity];
}

int64 CycleClock_Now() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<int64>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

const size_t LogMessage::kMaxLogMessageLen = 30000;

struct LogMessage::LogMessageData {
  LogMessageData();
  int preserved_errno_;
  char message_text_[LogMessage::kMaxLogMessageLen + 1];
  LogStream stream_;
  LogSeverity severity_;
  int line_;
  void (LogMessage::*send_method_)();
  time_t timestamp_;
  struct ::tm tm_time_;
  size_t num_prefix_chars_;
  size_t num_chars_to_log_;
  const char* basename_;
  const char* fullname_;
  const char* module_;
  bool has_been_flushed_;
  bool first_fatal_;
 private:
  DISALLOW_COPY_AND_ASSIGN(LogMessageData);
};

// A lock that allows only one thread to log at a time, to keep
// things from getting jumbled.
// Some other very uncommon logging operations(like changing the
// destination file for log messages of a given severity and module)
// also lock this mutex.
// Please be sure that anybody who might possibly need to lock it
// does so.
static Mutex log_mutex;

LogMessage::LogMessageData::LogMessageData()
  : stream_(message_text_, LogMessage::kMaxLogMessageLen, 0) {
}

std::ostream& LogMessage::stream() {
  return data_->stream_;
}
LogMessage::~LogMessage() {
  Flush();
  delete allocated_;
}

base::Logger::~Logger() {
}

// Each LogDestination has a member of type LogFileObject
class LogFileObject : public base::Logger {
public:
  LogFileObject(LogSeverity severity, const char* base_filename);
  ~LogFileObject();
  virtual void Write(bool force_flush,
                     time_t timestamp,
                     const char* message,
                     int message_len);
  virtual void Flush();
  // Internal flush routine.
  void FlushUnLocked();
  virtual uint32_t LogSize();
private:
  bool CreateLogFile(const std::string& time_pid_string);

  Mutex lock_;
  std::string base_filename_;
  FILE* file_;
  uint32 file_length_;
  uint32 bytes_since_flush_;
  int64 next_flush_time_;
  LogSeverity severity_;
};

LogFileObject::LogFileObject(LogSeverity severity,
                             const char* base_filename)
  :base_filename_((base_filename != NULL) ? base_filename : ""),
   file_(NULL),
   severity_(severity),
   file_length_(0),
   bytes_since_flush_(0),
   next_flush_time_(0) {
}

LogFileObject::~LogFileObject() {
  MutexLock l(lock_);
  if (file_ != NULL) {
    fclose(file_);
    file_ = NULL;
  }
}

DEFINE_int32(logbufsecs, 30,
                  "Buffer log messages for at most this many seconds");
void LogFileObject::FlushUnLocked() {
  if (file_ != NULL) {
    fflush(file_);
    bytes_since_flush_ = 0;
  }
  // Figure out when we are due for another flush.
  const int64 next = FLAGS_logbufsecs * static_cast<int64>(10000000);
  next_flush_time_ = CycleClock_Now() + next;
}

void LogFileObject::Flush() {
  MutexLock l(lock_);
  FlushUnLocked();
}

uint32 LogFileObject::LogSize() {
  MutexLock l(lock_);
  return file_length_;
}

bool LogFileObject::CreateLogFile(const std::string& time_pid_string) {
  const char* filename = base_filename_.c_str();
  // int fd = open(filename, O_WRONLY|O_CREAT|O_EXCL, 0664);
  int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0664);
  if (fd == -1) {
    return false;
  }
  fcntl(fd, F_SETFD, FD_CLOEXEC);
  file_ = fdopen(fd, "a");
  if (file_ == NULL) {
    close(fd);
    unlink(filename);
    return false;
  }
  return true;
}

void LogFileObject::Write(bool force_flush,
                          time_t timestamp,
                          const char* message,
                          int message_len) {
  MutexLock l(lock_);
  if (base_filename_.empty()) {
    return;
  }
  // TODO(geshuning): limit the totol size
  if (file_ == NULL) {
    if (!CreateLogFile("")) {   // NOT use the parameter
      perror("Could not create log file");
    } else {
      struct ::tm tm_time;
      localtime_r(&timestamp, &tm_time);
      std::ostringstream file_header_stream;
      file_header_stream.fill('0');
      file_header_stream << "Log file created at:"
                         << 1900 + tm_time.tm_year << '/'
                         << std::setw(2) << 1 + tm_time.tm_mon << '/'
                         << std::setw(2) << tm_time.tm_mday
                         << ' '
                         << std::setw(2) << tm_time.tm_hour << ':'
                         << std::setw(2) << tm_time.tm_min << ':'
                         << std::setw(2) << tm_time.tm_sec << '\n' << '\n';
      const std::string& file_header_string = file_header_stream.str();
      const int header_len = file_header_string.size();
      fwrite(file_header_string.data(), 1, header_len, file_);
      file_length_ += header_len;
      bytes_since_flush_ += header_len;
    }
  }

  // Write log to file
  errno = 0;
  fwrite(message, 1, message_len, file_);
  if (errno == ENOSPC) {
    // TODO(geshuning):some protections if disk is full.
    return;
  } else {
    file_length_ += message_len;
    bytes_since_flush_ += message_len;
  }

  // Whether force flush
  if (force_flush
      || (bytes_since_flush_ >= 1000000)
      || (CycleClock_Now() >= next_flush_time_)) {
    FlushUnLocked();
  }
}

class LogDestination {
public:
  friend class LogMessage;

  static void SetLogDestination(LogSeverity severity, const char* base_filename);
  static void FlushLogFiles(int min_severity);
  static void DeleteLogDestinations();
  static void MaybeLogToLogfile(const char* module,
                                LogSeverity severity, time_t timestamp,
                                const char* message, size_t len);
private:
  LogDestination(LogSeverity severity, const char* base_filename);
  ~LogDestination() {}
  static void LogToAllLogfiles(LogSeverity severity,
                               time_t timestamp,
                               const char* message, size_t len);
  LogFileObject fileobject_;
  base::Logger* logger_;        // the pointer of fileobject_
  DISALLOW_COPY_AND_ASSIGN(LogDestination);
};

typedef std::map<std::string, LogDestination*> dest_map_type;
dest_map_type dest_map;

void LogDestination::DeleteLogDestinations() {
  dest_map_type::iterator it = dest_map.begin();
  for (; it != dest_map.end(); ++it) {
    delete it->second;
    it->second = NULL;
  }
  dest_map.clear();
}

void LogDestination::FlushLogFiles(int min_severity) {
  // Prevent any subtle race conditions by wrapping a mutex lock
  // around all this stuff.
  MutexLock l(log_mutex);
  dest_map_type::iterator it = dest_map.begin();
  for (; it != dest_map.end(); ++it) {
    it->second->logger_->Flush();
  }
}

LogDestination::LogDestination(LogSeverity severity,
                               const char* base_filename)
  : fileobject_(severity, base_filename),
    logger_(&fileobject_) {
}

void LogDestination::MaybeLogToLogfile(const char* module,
                                       LogSeverity severity, time_t timestamp,
                                       const char* message, size_t len) {

  LogDestination* dest = NULL;
  const char* dest_file = NULL;
  if (module != NULL) {
    dest_file = module;
  } else {
    dest_file = GetLogSeverityName(severity);
  }
  dest_map_type::iterator it = dest_map.find(dest_file);
  if (it != dest_map.end()) {
    dest = it->second;
  } else {
    dest = new LogDestination(severity, dest_file);
    dest_map[dest_file] = dest;
  }
  dest->logger_->Write(true, timestamp, message, len);
}

static bool exit_on_dfatal = true;

void SetExitOnDFatal(bool value) {
  MutexLock l(log_mutex);
  exit_on_dfatal = value;
}

bool GetExitOnDFatal(bool value) {
  MutexLock l(log_mutex);
  return exit_on_dfatal;
}

void LogMessage::SendToLog() {
  // TODO(geshuning): implementation of AssertHeld()
  log_mutex.AssertAcquired();
  //log_mutex.AssertHeld();
  LogDestination::MaybeLogToLogfile(data_->module_,
                                    data_->severity_, data_->timestamp_,
                                    data_->message_text_, data_->num_chars_to_log_);
  if (data_->severity_ == kLS_FATAL && exit_on_dfatal) {
    // TODO(geshuning): create fatal message.
    log_mutex.Release();
    Fail();
  }
}


static LogMessage::LogMessageData fatal_msg_data_exclusive;
static Mutex fatal_msg_lock;
void LogMessage::Init(const char* module, const char* file,int line, LogSeverity severity,
                      void (LogMessage::*send_method)()) {
  allocated_ = NULL;
  if (severity != kLS_FATAL) {
    allocated_ = new LogMessageData();
    data_ = allocated_;  // just another pointer named better.
    data_->first_fatal_ = false;
  } else {
    MutexLock l(fatal_msg_lock);
    data_ = &fatal_msg_data_exclusive;
    data_->first_fatal_ = true;
    // TODO(geshuning): record the crash reason
    // TODO(geshuning): use shared_fatal_msg or exclusive_fatal_msg
  }
  stream().fill('0');
  data_->severity_ = severity;
  data_->line_ = line;
  data_->module_ = module;
  data_->send_method_ = send_method;
  double now = CycleClock_Now() * 0.000001;
  data_->timestamp_ = static_cast<time_t>(now);
  localtime_r(&data_->timestamp_, &data_->tm_time_);
  int usecs = static_cast<int> ((now - data_->timestamp_) * 1000000);
  data_->num_chars_to_log_ = 0;
  data_->fullname_ = file;
  data_->basename_ = file;
  data_->has_been_flushed_ = false;
  stream() << LogSeverityNames[severity][0]
           << std::setw(2) << 1 + data_->tm_time_.tm_mon
           << std::setw(2) << data_->tm_time_.tm_mday
           << ' '
           << std::setw(2) << data_->tm_time_.tm_hour << ':'
           << std::setw(2) << data_->tm_time_.tm_min << ':'
           << std::setw(2) << data_->tm_time_.tm_sec << ':'
           << std::setw(6) << usecs
           << ' '
           << std::setfill(' ') << std::setw(5)
           << static_cast<unsigned int> (pthread_self()) << std::setfill('0')
           << ' '
           << data_->basename_ << ':' << data_->line_ << "] ";
  data_->num_prefix_chars_ = data_->stream_.pcount();
}

LogMessage::LogMessage(const char* file, int line, const CheckOpString& result) {
  Init(NULL, file, line, kLS_FATAL, &LogMessage::SendToLog);
  stream() << "Check Failed:" << (*result.str_) << "";
}

LogMessage::LogMessage(const char* file, int line) {
  Init(NULL, file, line, kLS_INFO, &LogMessage::SendToLog);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity) {
  Init(NULL, file, line, severity, &LogMessage::SendToLog);
}

LogMessage::LogMessage(const char* module, const char* file, int line) {
  Init(module, file, line, kLS_INFO, &LogMessage::SendToLog);
}

LogMessage::LogMessage(const char* module, const char* file, int line,
                       LogSeverity severity) {
  Init(module, file, line, severity, &LogMessage::SendToLog);
}

void LogMessage::Flush() {
  if (data_->has_been_flushed_) { // TODO(geshuning): has_been_flushed_
    return;
  }
  data_->num_chars_to_log_ = data_->stream_.pcount();
  bool append_newline = (data_->message_text_[data_->num_chars_to_log_-1] != '\n');
  char original_final_char = '\0';
  if (append_newline) {
    original_final_char = data_->message_text_[data_->num_chars_to_log_];
    data_->message_text_[data_->num_chars_to_log_++] = '\n';
  }

  {
    MutexLock l(log_mutex);
    (this->*(data_->send_method_))();
  }

  if (append_newline) {
    data_->message_text_[data_->num_chars_to_log_-1] = original_final_char;
  }
  if (data_->preserved_errno_ != 0) {
    errno = data_->preserved_errno_;
  }
  data_->has_been_flushed_ = true;
}

void LogMessage::Fail() {
  // TODO(geshuning): some user-defined fail-functions
  abort();
}

LogMessageFatal::LogMessageFatal(const char* file, int line)
  : LogMessage(file, line, kLS_FATAL){
}

LogMessageFatal::LogMessageFatal(const char* file, int line,
                                 const CheckOpString& result)
  : LogMessage(file, line ,result) {
}

LogMessageFatal::~LogMessageFatal() {
  Flush();
  LogMessage::Fail();
}


// global functions
static const char* g_log_dir = "/tmp";
static const char* g_program_invocation_short_name = NULL;
static pid_t g_program_invocation_pid = 0;
void InitLoggingUtilities(const char* argv0) {
  const char* slash = strrchr(argv0, '/');
  g_program_invocation_short_name = slash ? slash + 1 : argv0;
  g_program_invocation_pid = getpid();
  // TODO(geshuning): install failure function
  // InstallFailuerFunction()
}
