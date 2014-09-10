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

#ifndef BASE_SYNCHRONIZATION_LOCK_HH_
#define BASE_SYNCHRONIZATION_LOCK_HH_

#include <pthread.h>
#include <unistd.h>
#include "base/basictypes.hh"

namespace base {
namespace internal {
class LockImpl {
public:
    typedef pthread_mutex_t NativeHandle;
    LockImpl();
    ~LockImpl();
    bool Try();
    void Lock();
    void Unlock();
    NativeHandle *native_handle() {
        return &native_handle_;
    }
private:
    NativeHandle native_handle_;
    DISALLOW_COPY_AND_ASSIGN(LockImpl);
};
}  // namespace internal

class Lock {
public:
    Lock() : lock_() {
    }

    ~Lock() {
    }

    void Acquire() {
        lock_.Lock();
    }

    void Release() {
        lock_.Unlock();
    }

    // If the lock is not held, take it and return true. If the lock is already
    // held by another thread, immediately return false. This must not be called
    // by a thread already holding the lock (what happens is undefined and an
    // assertion may fail).
    bool Try() {
        return lock_.Try();
    }

    void AssertAcquired() const {
        // TODO(gene.ge)
    }

private:
    internal::LockImpl lock_;
};

class AutoLock {
public:
    // struct AlreadyAcquired {};
    explicit AutoLock(Lock &lock) : lock_(lock) {
        lock_.Acquire();
    }

    ~AutoLock() {
        lock_.Release();
    }

private:
    Lock &lock_;
};

}

#endif  // BASE_SYNCHRONIZATION_LOCK_HH_
