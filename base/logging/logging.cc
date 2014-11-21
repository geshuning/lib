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

LOG_DEFINE_THIS_MODULE(log);

const char* const LogSeverityNames[] = {
#define LOG_SEVERITY(NAME,SEVERITY)#NAME,
    LOG_SEVERITYS
#undef LOG_SEVERITY
};
const char* GetLogSeverityName(LogSeverity severity) {
  return LogSeverityNames[severity];
}

// LogModule
static std::list<LogModule*> module_list;
LogModule::LogModule(const std::string m_name) :
        name(m_name),min_severity(kLS_INFO),
        vlog_on(true), n_bytes(0), max_bytes(kuint32max)
{
    module_list.push_back(this);
}
void LogModule::AddLogDestination(LogDestination *dst, LogSeverity severity)
{
    severity_dsts[severity][dst->type] = dst;
}

// LogMessageData
const size_t LogMessage::kMaxLogMessageLen = 30000;
struct LogMessage::LogMessageData {
    LogMessageData();
    int preserved_errno_;
    char message_text_[LogMessage::kMaxLogMessageLen + 1];
    LogStream stream_;
    LogSeverity severity_;
    int line_;
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

// LogDestination
DEFINE_int32(logbufsecs, 30,
             "Buffer log messages for at most this many seconds");
LogDestinationToFile::LogDestinationToFile(std::string name) :
        base_filename_(name),file_(NULL),
        file_length_(0),
        bytes_since_flush_(0),bytes_max_flush_(kuint32max),
        next_flush_time_(0)
{
}

LogDestinationToFile::~LogDestinationToFile()
{
    MutexLock l(lock_);
    if (file_ != NULL) {
        // TODO(geshuning): close here, how about fd;
        fclose(file_);
        file_ = NULL;
    }
}

void LogDestinationToFile::FlushUnLocked()
{
    if (file_ != NULL) {
        fflush(file_);
        bytes_since_flush_ = 0;
    }
    const int64 next = FLAGS_logbufsecs * static_cast<int64>(10000000);
    next_flush_time_ = base::Time::Now().ToInternalValue()+ next;
}

void LogDestinationToFile::Log(LogSeverity severity, time_t timestamp,
                               const char* message, size_t len)
{
    MutexLock l(lock_);
    if (base_filename_.empty()) {
        return;
    }
    if (file_ == NULL) {
        const char* filename = base_filename_.c_str();
        log_fd_ = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0664);
        if (log_fd_ == -1) {
            LOG_WARN() << "Can not open file " << base_filename_;
            return;
        }
        fcntl(log_fd_, F_SETFD, FD_CLOEXEC);
        file_ = fdopen(log_fd_, "a");
        if (file_ == NULL) {
            close(log_fd_);
            unlink(filename);
            LOG_WARN() << "Faild to do fdopen with " << base_filename_;
        }
    }
    if (file_ == NULL) {
        return;
    }
    if (bytes_since_flush_ > bytes_max_flush_) {
        LOG_WARN() << base_filename_ << "had exceed its max length("
                   << bytes_max_flush_ << "bytes)";
        return;
    }
    // TODO(gene.ge): use time struct
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
    //TODO(gene.ge):change to aio
    fwrite(file_header_string.data(), 1, header_len, file_);
    file_length_ += header_len;
    bytes_since_flush_ += header_len;

    // Write log to file
    errno = 0;
    fwrite(message, 1, len, file_);
    if (errno == ENOSPC) {
        LOG_WARN() << "Disk full";
        return;
    } else {
        file_length_ += len;
        bytes_since_flush_ += len;
    }

    if ((bytes_since_flush_ >= 1000000)
        || (base::Time::Now().ToInternalValue()>= next_flush_time_)) {
        FlushUnLocked();
    }
}

// A lock that allows only one thread to log at a time, to keep
// things from getting jumbled.
// Some other very uncommon logging operations(like changing the
// destination file for log messages of a given severity and module)
// also lock this mutex.
// Please be sure that anybody who might possibly need to lock it
// does so.
static Mutex log_mutex;
LogMessage::LogMessageData::LogMessageData() :
        stream_(message_text_, LogMessage::kMaxLogMessageLen, 0)
{
}

std::ostream& LogMessage::stream()
{
    return data_->stream_;
}

LogMessage::~LogMessage()
{
    Flush();
    delete allocated_;
}

static LogMessage::LogMessageData fatal_msg_data_exclusive;
static Mutex fatal_msg_lock;
void LogMessage::Init(const LogModule *module,
                      const char* file,int line, LogSeverity severity)
{

    std::cout << "here" << std::endl;
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
    std::cout << "here" << std::endl;
    stream().fill('0');
    data_->severity_ = severity;
    data_->line_ = line;
    data_->module_ = module->name.c_str();
    double now = base::Time::Now().ToInternalValue() * 0.000001;
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
//TODO(gene.ge): what this for??
/*
LogMessage::LogMessage(const char* file, int line, const CheckOpString& result) {
  //Init(NULL, file, line, kLS_FATAL, &LogMessage::SendToLog);
  stream() << "Check Failed:" << (*result.str_) << "";
}
*/

LogMessage::LogMessage(const LogModule *module, const char* file, int line,
                       LogSeverity severity)
{
    if (module->n_bytes >= module->max_bytes) {
        LOG_WARN() << "module(" << module->name << ")'s max size has exceed("
                   << module->max_bytes << ")";
    } else {
        // TODO(gene.ge): add a member to indicate whether should be write
        Init(module, file, line, severity);
    }
}
void LogMessage::Flush() {
  if (data_->has_been_flushed_) { // TODO(geshuning): has_been_flushed_
    return;
  }
  data_->num_chars_to_log_ = data_->stream_.pcount();
  bool append_newline =
          (data_->message_text_[data_->num_chars_to_log_-1] != '\n');
  char original_final_char = '\0';
  if (append_newline) {
    original_final_char = data_->message_text_[data_->num_chars_to_log_];
    data_->message_text_[data_->num_chars_to_log_++] = '\n';
  }

  {
    MutexLock l(log_mutex);
    std::cout << module_->name << std::endl;
    for (int i = 0; i < kLOG_DST_MAX; ++i) {
        LogDestination *dst = module_->severity_dsts[data_->severity_][i];
        if (dst) {
            dst->Log(data_->severity_, data_->timestamp_,
                     data_->message_text_, data_->num_chars_to_log_);
        }
    }
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
/*
LogMessageFatal::LogMessageFatal(const char* file, int line) {
//  : LogMessage(file, line, kLS_FATAL){
}

LogMessageFatal::LogMessageFatal(const char* file, int line,
                                 const CheckOpString& result) {
 // : LogMessage(file, line ,result) {
}

LogMessageFatal::~LogMessageFatal() {
  Flush();
  LogMessage::Fail();
}
*/

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
