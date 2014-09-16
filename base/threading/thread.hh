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

#ifndef BASE_THREADING_THREAD_HH_
#define BASE_THREADING_THREAD_HH_

#include <pthread.h>

#include "base/basictypes.hh"

namespace base {
class Thread {
public:
    explicit Thread(const std::string &name) :
            name_(name) {}
    virtual ~Thread();

    bool Start();
    // bool StartWithOptions(const Options &options);
    void Stop();
    const std::string &ThreadName() const {
        return name_;
    }
    bool IsRunning() const;
    void SetPriority(int priority);
    void SetAffinity(int core_id) {
        pthread_attr_t attr;
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);
        pthread_attr_init(&attr);
        pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);
    }

protected:
    virtual void Init() {}
    virtual void Run() {}

private:
    bool running_;
    bool stopping_;
    bool joinable_;
    std::string name_;
    DISALLOW_COPY_AND_ASSIGN(Thread);
};
}      // namespace base
#endif  // BASE_THREADING_THREAD_HH_
