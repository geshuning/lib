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

#ifndef BASE_MEMORY_ATOMIC_H
#define BASE_MEMORY_ATOMIC_H

// TODO(gene.ge): implement this
namespace base {

namespace subtle {
    typedef int32 Atomic32;
    typedef intptr_t AtomicWord;
}  // namespace subtle

typedef subtle::Atomic32 AtomicRefCount;
inline void AtomicRefCountIncN(volatile AtomicRefCount *ptr,
                               AtomicRefCount increment)
{
}

inline bool AtomicRefCountDecN(volatile AtomicRefCount *ptr,
                               AtomicRefCount decrement)
{
}

inline void AtomicRefCountInc(volatile AtomicRefCount *ptr)
{
    base::AtomicRefCountIncN(ptr, 1);
}

inline void AtomicRefCountDec(volatile AtomicRefCount *ptr)
{
    base::AtomicRefCountDecN(ptr, 1);
}

}  // namespace base

#endif  // BASE_MEMORY_ATOMIC_H
