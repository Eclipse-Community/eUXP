/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZILLA_FRAMEPOOL_H_
#define MOZILLA_FRAMEPOOL_H_

#include "mozilla/Atomics.h"
#include "mozilla/Mutex.h"
#include "mozilla/RefPtr.h"
#include "nsTArray.h"
#include "gfxTypes.h"

namespace mozilla {

namespace layers {
class Image;
} // namespace layers

/**
 * FramePool manages a pool of reusable video frame buffers to reduce
 * memory allocation/deallocation overhead during playback.
 *
 * Usage:
 *   RefPtr<FramePool> pool = new FramePool(max_frames);
 *   RefPtr<layers::Image> frame = pool->AcquireFrame(size);
 *   // use frame...
 *   pool->ReleaseFrame(frame);  // returns to pool or deallocates
 */
class FramePool {
public:
  explicit FramePool(size_t aMaxFrames = 16);
  private:
    ~FramePool();

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FramePool)

  /**
   * Acquire a frame buffer from the pool. If no pooled frames are available,
   * allocates a new one. Thread-safe.
   */
  RefPtr<layers::Image> AcquireFrame(const gfx::IntSize& aSize);

  /**
   * Return a frame to the pool for reuse. If pool is full, frame is released.
   * Thread-safe.
   */
  void ReleaseFrame(layers::Image* aFrame);

  /**
   * Clear all pooled frames and release memory.
   */
  void Clear();

  /**
   * Get current number of frames in pool.
   */
  size_t PoolSize() const;

  /**
   * Get total frames allocated (pooled + in use).
   */
  size_t TotalFrames() const;

  /**
   * Get statistics about pool usage.
   */
  struct Stats {
    size_t mPooledFrames;     // Frames available in pool
    size_t mAllocatedFrames;  // Total frames ever allocated
    size_t mReusedFrames;     // Frames reused from pool
    size_t mPeakPoolSize;     // Maximum simultaneous frames in pool
  };

  Stats GetStats() const {
     MutexAutoLock lock(mMutex);
     return {mFramePool.Length(), mTotalAllocated, mFramesReused, mPeakPoolSize};
  }

  /**
   * Reset statistics counters.
   */
  void ResetStats() {
      MutexAutoLock lock(mMutex);
      mFramesReused = 0;
      mTotalAllocated = 0;
      mPeakPoolSize = 0;
  }

private:
  mutable Mutex mMutex;
  nsTArray<RefPtr<layers::Image>> mFramePool;
  const size_t mMaxPoolSize;
  
  // Statistics
  size_t mTotalAllocated = 0;
  size_t mFramesReused = 0;
  size_t mPeakPoolSize = 0;
};

} // namespace mozilla

#endif // MOZILLA_FRAMEPOOL_H_
