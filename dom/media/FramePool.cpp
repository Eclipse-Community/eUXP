/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FramePool.h"
#include "mozilla/Logging.h"
#include "ImageContainer.h"

namespace mozilla {

extern LazyLogModule gMediaDecoderLog;

#undef LOG
#define LOG(arg, ...) MOZ_LOG(gMediaDecoderLog, mozilla::LogLevel::Debug, ("FramePool::%s: " arg, __func__, ##__VA_ARGS__))
#define LOGV(arg, ...) MOZ_LOG(gMediaDecoderLog, mozilla::LogLevel::Verbose, ("FramePool::%s: " arg, __func__, ##__VA_ARGS__))

FramePool::FramePool(size_t aMaxFrames)
  : mMutex("FramePool::mMutex")
  , mMaxPoolSize(aMaxFrames > 0 ? aMaxFrames : 16)
{
  LOG("FramePool created with max size: %zu", mMaxPoolSize);
}

FramePool::~FramePool()
{
  Clear();
}

RefPtr<layers::Image> FramePool::AcquireFrame(const gfx::IntSize& aSize)
{
  MutexAutoLock lock(mMutex);

  // Try to reuse a frame from pool
  if (!mFramePool.IsEmpty()) {
    RefPtr<layers::Image> frame = mFramePool[mFramePool.Length() - 1];
    mFramePool.RemoveElementAt(mFramePool.Length() - 1);
    mFramesReused++;
    LOGV("Reused frame from pool, pool size now: %zu", mFramePool.Length());
    return frame;
  }

  // No pooled frame available; we cannot reliably construct a specific
  // Image subclass here (requires platform-specific recycle bin). Return
  // nullptr and let the caller allocate an appropriate Image if needed.
  mTotalAllocated++;
  LOG("No pooled frame available, returning nullptr (alloc attempts: %zu)", mTotalAllocated);
  return nullptr;
}

void FramePool::ReleaseFrame(layers::Image* aFrame)
{
  if (!aFrame) {
    return;
  }

  MutexAutoLock lock(mMutex);

  // Only pool frame if we haven't reached max capacity
  if (mFramePool.Length() < mMaxPoolSize) {
    mFramePool.AppendElement(RefPtr<layers::Image>(aFrame));

    // Track peak pool size
    if (mFramePool.Length() > mPeakPoolSize) {
      mPeakPoolSize = mFramePool.Length();
    }

    LOGV("Released frame to pool, pool size: %zu/%zu", 
         mFramePool.Length(), mMaxPoolSize);
  } else {
    // Frame exceeds pool capacity, let it be released
    LOGV("Frame pool at capacity, releasing frame");
  }
}

void FramePool::Clear()
{
  MutexAutoLock lock(mMutex);
  mFramePool.Clear();
  LOG("Frame pool cleared");
}

size_t FramePool::PoolSize() const
{
  MutexAutoLock lock(mMutex);
  return mFramePool.Length();
}

size_t FramePool::TotalFrames() const
{
  MutexAutoLock lock(mMutex);
  return mTotalAllocated;
}

} // namespace mozilla
