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

#ifndef BASE_MEMORY_REF_COUNTED_HH_
#define BASE_MEMORY_REF_COUNTED_HH_

#include "base/memory/atomic.hh"

namespace base {
namespace subtle {

class RefCountedBase {
public:
    bool HasOneRef() const {
        return ref_count_ == 1;
    }
protected:
    RefCountedBase() : ref_count_(0) {
    }

    ~RefCountedBase() {
    }

    void AddRef() const {
        ++ref_count_;
    }

    bool Release() const {
        if (--ref_count_ == 0) {
            return true;
        }
        return false;
    }

private:
    mutable int ref_count_;
    DISALLOW_COPY_AND_ASSIGN(RefCountedBase);
};

class RefCountedThreadSafeBase {
public:
    bool HasOneRef() const {
        return AtomicRefCountIsOne(
        &const_cast<RefCountedThreadSafeBase*>(this)->ref_count_);
    }
protected:
    RefCountedThreadSafeBase() : ref_count_(0) {
    }

    ~RefCountedThreadSafeBase() {
    }

    void AddRef() const {
        AtomicRefCountInc(&ref_count_);
    }

    bool Release() const {
        if (!AtomicRefCountDec(&ref_count_)) {
            return true;
        }
        return false;
    }

private:
    mutable AtomicRefCount ref_count_;
    DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafeBase);
};

}  // namespace subtle

// A base class for reference counted classes.
// Usage:
//   class MyFoo : public base::RefCounted<MyFoo> {
//    ...
//    private:
//     friend class base::RefCounted<MyFoo>;
//     ~MyFoo();
//   };
// You should always make your destructor private, to avoid any code deleting
// the object accidently while there are references to it.
template <class T>
class RefCounted : public subtle::RefCountedBase {
public:
    RefCounted() {}

    void AddRef() const {
        subtle::RefCountedBase::AddRef();
    }

    void Release() const {
        if (subtle::RefCountedBase::Release()) {
            delete static_cast<const T*>(this);
        }
    }

protected:
    ~RefCounted() {}

private:
    DISALLOW_COPY_AND_ASSIGN(RefCounted<T>);
};

// Forward declaration.
template <class T, typename Traits> class RefCountedThreadSafe;

// Default traits for RefCountedThreadSafe<T>.  Deletes the object when its ref
// count reaches 0.  Overload to delete it on a different thread etc.
template<typename T>
struct DefaultRefCountedThreadSafeTraits {
    static void Destruct(const T* x) {
        // Delete through RefCountedThreadSafe to make child classes only
        // need to be friend with RefCountedThreadSafe instead of this struct,
        // which is an implementation detail.
        RefCountedThreadSafe<T,
                             DefaultRefCountedThreadSafeTraits>::DeleteInternal(x);
    }
};

// A thread-safe variant of RefCounted<T>
//
//   class MyFoo : public base::RefCountedThreadSafe<MyFoo> {
//    ...
//   };
//
// If you're using the default trait, then you should add compile time
// asserts that no one else is deleting your object.  i.e.
//    private:
//     friend class base::RefCountedThreadSafe<MyFoo>;
//     ~MyFoo();
template <class T, typename Traits = DefaultRefCountedThreadSafeTraits<T> >
class RefCountedThreadSafe : public subtle::RefCountedThreadSafeBase {
public:
    RefCountedThreadSafe() {}

    void AddRef() const {
        subtle::RefCountedThreadSafeBase::AddRef();
    }

    void Release() const {
        if (subtle::RefCountedThreadSafeBase::Release()) {
            Traits::Destruct(static_cast<const T*>(this));
        }
    }

protected:
    ~RefCountedThreadSafe() {}

private:
    friend struct DefaultRefCountedThreadSafeTraits<T>;
    static void DeleteInternal(const T* x) {
        delete x;
    }

    DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafe);
};

//
// A thread-safe wrapper for some piece of data so we can place other
// things in scoped_refptrs<>.
//
template<typename T>
class RefCountedData
        : public base::RefCountedThreadSafe< base::RefCountedData<T> > {
public:
    RefCountedData() : data() {}
    RefCountedData(const T& in_value) : data(in_value) {}

    T data;

private:
    friend class base::RefCountedThreadSafe<base::RefCountedData<T> >;
    ~RefCountedData() {}
};

}  // namespace base

// A smart pointer class for reference counted objects,
// Use this class instead of calling AddRef and Release
// manually on a reference counted object to void memory leak.
template <typename T>
class scoped_refptr {
  public:
  typedef T element_type;
  scoped_refptr() : ptr_(NULL) {
  }
  scoped_refptr(T* p) : ptr_(p) {
    if (ptr_) {
      ptr_->AddRef();
    }
  }
  scoped_refptr(const scoped_refptr<T>& r) : ptr_(r.ptr_) {
    if (ptr_) {
      ptr_->AddRef();
    }
  }
  template <typename U>
  scoped_refptr(const scoped_refptr<U>& r) : ptr_(r.get()) {
    if (ptr_) {
      ptr_->AddRef();
    }
  }
  ~scoped_refptr() {
    if (ptr_) {
      ptr_->Release();
    }
  }
  T* get() const { return ptr_; }
  operator T*()const {return ptr_; }
  T* operator->() const {
    assert(ptr_ != NULL);
    return ptr_;
  }
  scoped_refptr<T>& operator=(T* p) {
    if (p) {
      p->AddRef();
    }
    T* old_ptr = ptr_;
    ptr_ = p;
    if (old_ptr) {
      old_ptr->Release();
    }
    return *this;
  }
  scoped_refptr<T>& operator=(const scoped_refptr<T>& r) {
    return *this = r.ptr_;
  }
  template <typename U>
  scoped_refptr<T>& operator=(const scoped_refptr<U>& r) {
    return *this = r.ptr_;
  }
  void swap(T** pp) {
    T* p = ptr_;
    ptr_ = *pp;
    *pp = p;
  }
  void swap(scoped_refptr<T>& r) {
    swap(&r.ptr_);
  }

  protected:
  T* ptr_;
};

// Handy utility for creating a scoped_refptr<T> out of a T* explicitly without
// having to retype all the template arguments
template <typename T>
scoped_refptr<T> make_scoped_refptr(T* t) {
  return scoped_refptr<T>(t);
}

#endif  // BASE_MEMORY_REF_COUNTED_HH_
