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

#include "base/synchronization/lock.hh"

#include <errno.h>
#include <string.h>

namespace base {
namespace internal {
LockImpl::LockImpl()
{
    pthread_mutex_init(&native_handle_, NULL);
}

LockImpl::~LockImpl()
{
    pthread_mutex_destroy(&native_handle_);
}

bool LockImpl::Try()
{
    int rv = pthread_mutex_trylock(&native_handle_);
    // DCHECK(rv == 0 || rv == EBUSY) << ". " << strerror(rv);
    return rv == 0;
}

void LockImpl::Lock()
{
    pthread_mutex_lock(&native_handle_);
}

void LockImpl::Unlock()
{
    int rv = pthread_mutex_unlock(&native_handle_);
}

}  // namespace internal
}  // namespace base
