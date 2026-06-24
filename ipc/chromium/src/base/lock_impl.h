// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_LOCK_IMPL_H_
#define BASE_LOCK_IMPL_H_

#include "base/basictypes.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>

// Struct to hold either SRW lock (Win7+) or CRITICAL_SECTION (Vista fallback)
// We use a struct instead of union to avoid memory corruption from overlapping fields
#define LOCK_TYPE_SRWLOCK 0
#define LOCK_TYPE_CRITICALECTION 1

struct NativeHandleWin {
  int type;  // Discriminator: LOCK_TYPE_SRWLOCK or LOCK_TYPE_CRITICALECTION
  union {
    SRWLOCK srwlock;
    CRITICAL_SECTION critical_section;
  } handle;
  
  NativeHandleWin() : type(LOCK_TYPE_SRWLOCK), handle() {
    // Constructor initializes type and zero-initializes the union
  }
};

#elif defined(OS_POSIX)
#include <pthread.h>
#endif

namespace base {
namespace internal {

// This class implements the underlying platform-specific spin-lock mechanism
// used for the Lock class.  Most users should not use LockImpl directly, but
// should instead use Lock.
class LockImpl {
 public:
#if defined(OS_WIN)
  using NativeHandle = NativeHandleWin;
#elif defined(OS_POSIX)
  using NativeHandle =  pthread_mutex_t;
#endif

  LockImpl();
  ~LockImpl();

  // If the lock is not held, take it and return true.  If the lock is already
  // held by something else, immediately return false.
  bool Try();

  // Take the lock, blocking until it is available if necessary.
  void Lock();

  // Release the lock.  This must only be called by the lock's holder: after
  // a successful call to Try, or a call to Lock.
  void Unlock();

  // Return the native underlying lock.  
  // TODO(awalker): refactor lock and condition variables so that this is
  // unnecessary.
  NativeHandle* native_handle() { return &native_handle_; }

#if defined(OS_WIN)
  SRWLOCK* native_srwlock() { return &native_handle_.handle.srwlock; }
  CRITICAL_SECTION* native_critical_section() {
    return &native_handle_.handle.critical_section;
  }
  bool uses_critical_section() const {
    return native_handle_.type == LOCK_TYPE_CRITICALECTION;
  }
#endif

#if defined(OS_POSIX)
  // Whether this lock will attempt to use priority inheritance.
  static bool PriorityInheritanceAvailable();
#endif

 private:
  NativeHandle native_handle_;

  DISALLOW_COPY_AND_ASSIGN(LockImpl);
};

}  // namespace internal
}  // namespace base

#endif  // BASE_LOCK_IMPL_H_
