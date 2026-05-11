/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZILLA_VIDEOPLAYBACKSTATS_H_
#define MOZILLA_VIDEOPLAYBACKSTATS_H_

#include "mozilla/Atomics.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "TimeUnits.h"

namespace mozilla {

/**
 * Tracks performance and memory statistics for video playback.
 * Useful for debugging performance issues and optimizing buffering policies.
 */
class VideoPlaybackStats {
public:
  struct FrameStats {
    // Timing information
    TimeStamp mDecodeStart;
    TimeStamp mDecodeEnd;
    int64_t mDecodeTimeUs = 0;
    int64_t mDisplayTimeUs = 0;
    
    // Frame properties
    bool mIsKeyframe = false;
    bool mWasSkipped = false;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    uint64_t mMemoryBytes = 0;
  };

  struct PlaybackStats {
    // Frame statistics
    uint64_t mTotalFramesDecoded = 0;
    uint64_t mTotalFramesDisplayed = 0;
    uint64_t mTotalFramesSkipped = 0;
    uint64_t mTotalFramesDropped = 0;
    
    // Memory statistics (peak values)
    uint64_t mPeakMemoryUsage = 0;
    uint64_t mAverageMemoryUsage = 0;
    uint64_t mTotalMemoryAllocated = 0;
    
    // Performance metrics
    double mAverageDecodeTimeMs = 0.0;
    double mMaxDecodeTimeMs = 0.0;
    int32_t mFramesLateByMs = 0;
    
    // Buffer statistics
    uint32_t mAverageQueueSize = 0;
    uint32_t mPeakQueueSize = 0;
    
    // Hardware acceleration info
    bool mIsHardwareAccelerated = false;
    uint32_t mHardwareSkipCount = 0;
  };

  VideoPlaybackStats();
  ~VideoPlaybackStats();

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VideoPlaybackStats)

  /**
   * Record a decoded frame and its timing information.
   */
  void RecordFrameDecoded(const FrameStats& aStats);

  /**
   * Record a displayed frame.
   */
  void RecordFrameDisplayed(int64_t aDisplayTimeUs, uint32_t aQueueSize);

  /**
   * Record a skipped frame.
   */
  void RecordFrameSkipped(bool aIsKeyframe);

  /**
   * Record a dropped frame.
   */
  void RecordFrameDropped();

  /**
   * Record memory usage snapshot.
   */
  void RecordMemoryUsage(uint64_t aBytesUsed);

  /**
   * Set hardware acceleration status.
   */
  void SetHardwareAccelerated(bool aIsHWAccel) {
    mStats.mIsHardwareAccelerated = aIsHWAccel;
  }

  /**
   * Get current statistics snapshot.
   */
  PlaybackStats GetStats() const;

  /**
   * Reset all statistics (start fresh measurement).
   */
  void Reset();

  /**
   * Get human-readable statistics summary for debugging.
   */
  void GetDebugInfo(nsAString& aOutput) const;

private:
  mutable Mutex mMutex;
  PlaybackStats mStats;
  
  uint64_t mSampleCount = 0;                  // For computing averages
  uint64_t mTotalDecodeTimeUs = 0;
  uint64_t mTotalMemoryMeasurements = 0;
  uint64_t mMemoryAccumulator = 0;
};

} // namespace mozilla

#endif // MOZILLA_VIDEOPLAYBACKSTATS_H_
