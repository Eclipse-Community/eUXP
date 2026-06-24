// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/lock_impl.h"

namespace base {
namespace internal {

namespace {

typedef BOOLEAN(WINAPI* TryAcquireSRWLockExclusiveFn)(PSRWLOCK);
typedef void(WINAPI* AcquireSRWLockExclusiveFn)(PSRWLOCK);
typedef void(WINAPI* ReleaseSRWLockExclusiveFn)(PSRWLOCK);

TryAcquireSRWLockExclusiveFn GetTryAcquireSRWLockExclusive() {
  static TryAcquireSRWLockExclusiveFn fn = []() {
    HMODULE kernel32 = ::GetModuleHandleW(L"kernel32.dll");
    return reinterpret_cast<TryAcquireSRWLockExclusiveFn>(
        ::GetProcAddress(kernel32, "TryAcquireSRWLockExclusive"));
  }();
  return fn;
}

AcquireSRWLockExclusiveFn GetAcquireSRWLockExclusive() {
  static AcquireSRWLockExclusiveFn fn = []() {
    HMODULE kernel32 = ::GetModuleHandleW(L"kernel32.dll");
    return reinterpret_cast<AcquireSRWLockExclusiveFn>(
        ::GetProcAddress(kernel32, "AcquireSRWLockExclusive"));
  }();
  return fn;
}

ReleaseSRWLockExclusiveFn GetReleaseSRWLockExclusive() {
  static ReleaseSRWLockExclusiveFn fn = []() {
    HMODULE kernel32 = ::GetModuleHandleW(L"kernel32.dll");
    return reinterpret_cast<ReleaseSRWLockExclusiveFn>(
        ::GetProcAddress(kernel32, "ReleaseSRWLockExclusive"));
  }();
  return fn;
}

bool HasSRWLockSupport() {
  return GetTryAcquireSRWLockExclusive() && GetAcquireSRWLockExclusive() &&
         GetReleaseSRWLockExclusive();
}

}  // namespace

LockImpl::LockImpl() {
  if (HasSRWLockSupport()) {
    native_handle_.type = LOCK_TYPE_SRWLOCK;
    ::InitializeSRWLock(&native_handle_.handle.srwlock);
  } else {
    native_handle_.type = LOCK_TYPE_CRITICALECTION;
    ::InitializeCriticalSection(&native_handle_.handle.critical_section);
  }
}

LockImpl::~LockImpl() {
  if (native_handle_.type == LOCK_TYPE_CRITICALECTION) {
    ::DeleteCriticalSection(&native_handle_.handle.critical_section);
  }
}

bool LockImpl::Try() {
  if (native_handle_.type == LOCK_TYPE_CRITICALECTION) {
    return ::TryEnterCriticalSection(&native_handle_.handle.critical_section) != 0;
  }

  TryAcquireSRWLockExclusiveFn try_acquire = GetTryAcquireSRWLockExclusive();
  return try_acquire && !!try_acquire(&native_handle_.handle.srwlock);
}

void LockImpl::Lock() {
  if (native_handle_.type == LOCK_TYPE_CRITICALECTION) {
    ::EnterCriticalSection(&native_handle_.handle.critical_section);
  } else {
    AcquireSRWLockExclusiveFn acquire = GetAcquireSRWLockExclusive();
    if (acquire) {
      acquire(&native_handle_.handle.srwlock);
    }
  }
}

void LockImpl::Unlock() {
  if (native_handle_.type == LOCK_TYPE_CRITICALECTION) {
    ::LeaveCriticalSection(&native_handle_.handle.critical_section);
  } else {
    ReleaseSRWLockExclusiveFn release = GetReleaseSRWLockExclusive();
    if (release) {
      release(&native_handle_.handle.srwlock);
    }
  }
}

}  // namespace internal
}  // namespace base
