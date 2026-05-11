/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VideoPlaybackStats.h"
#include "mozilla/Logging.h"
#include "nsPrintfCString.h"

namespace mozilla {

extern LazyLogModule gMediaDecoderLog;

#define LOG(arg, ...) MOZ_LOG(gMediaDecoderLog, mozilla::LogLevel::Debug, ("VideoPlaybackStats::%s: " arg, __func__, ##__VA_ARGS__))

VideoPlaybackStats::VideoPlaybackStats()
{
  LOG("VideoPlaybackStats created");
}

VideoPlaybackStats::~VideoPlaybackStats()
{
  LOG("VideoPlaybackStats destroyed - Total frames decoded: %llu, displayed: %llu, skipped: %llu",
      mStats.mTotalFramesDecoded,
      mStats.mTotalFramesDisplayed,
      mStats.mTotalFramesSkipped);
}

void VideoPlaybackStats::RecordFrameDecoded(const FrameStats& aStats)
{
  MutexAutoLock lock(mMutex);
  
  mStats.mTotalFramesDecoded++;
  
  if (aStats.mDecodeTimeUs > 0) {
    mTotalDecodeTimeUs += aStats.mDecodeTimeUs;
    mSampleCount++;
    
    // Update average decode time
    mStats.mAverageDecodeTimeMs = (double)mTotalDecodeTimeUs / 
                                   (mSampleCount * 1000.0);
    
    // Track max decode time
    double decodeTimeMs = aStats.mDecodeTimeUs / 1000.0;
    if (decodeTimeMs > mStats.mMaxDecodeTimeMs) {
      mStats.mMaxDecodeTimeMs = decodeTimeMs;
    }
  }
  
  if (aStats.mMemoryBytes > 0) {
    if (aStats.mMemoryBytes > mStats.mPeakMemoryUsage) {
      mStats.mPeakMemoryUsage = aStats.mMemoryBytes;
    }
  }
}

void VideoPlaybackStats::RecordFrameDisplayed(int64_t aDisplayTimeUs, uint32_t aQueueSize)
{
  MutexAutoLock lock(mMutex);
  
  mStats.mTotalFramesDisplayed++;
  
  // Track queue statistics
  if (aQueueSize > mStats.mPeakQueueSize) {
    mStats.mPeakQueueSize = aQueueSize;
  }
  
  // Update rolling average queue size
  uint64_t total = mStats.mAverageQueueSize * mStats.mTotalFramesDisplayed;
  mStats.mAverageQueueSize = (total + aQueueSize) / (mStats.mTotalFramesDisplayed + 1);
}

void VideoPlaybackStats::RecordFrameSkipped(bool aIsKeyframe)
{
  MutexAutoLock lock(mMutex);
  mStats.mTotalFramesSkipped++;
}

void VideoPlaybackStats::RecordFrameDropped()
{
  MutexAutoLock lock(mMutex);
  mStats.mTotalFramesDropped++;
}

void VideoPlaybackStats::RecordMemoryUsage(uint64_t aBytesUsed)
{
  MutexAutoLock lock(mMutex);
  
  if (aBytesUsed > mStats.mPeakMemoryUsage) {
    mStats.mPeakMemoryUsage = aBytesUsed;
  }
  
  mMemoryAccumulator += aBytesUsed;
  mTotalMemoryMeasurements++;
  
  if (mTotalMemoryMeasurements > 0) {
    mStats.mAverageMemoryUsage = mMemoryAccumulator / mTotalMemoryMeasurements;
  }
}

VideoPlaybackStats::PlaybackStats VideoPlaybackStats::GetStats() const
{
  MutexAutoLock lock(mMutex);
  return mStats;
}

void VideoPlaybackStats::Reset()
{
  MutexAutoLock lock(mMutex);
  mStats = PlaybackStats();
  mSampleCount = 0;
  mTotalDecodeTimeUs = 0;
  mTotalMemoryMeasurements = 0;
  mMemoryAccumulator = 0;
  LOG("Statistics reset");
}

void VideoPlaybackStats::GetDebugInfo(nsAString& aOutput) const
{
  MutexAutoLock lock(mMutex);
  
  nsPrintfCString info(
    "Video Playback Statistics:\n"
    "  Frames decoded: %llu\n"
    "  Frames displayed: %llu\n"
    "  Frames skipped: %llu\n"
    "  Frames dropped: %llu\n"
    "  Average decode time: %.2f ms\n"
    "  Max decode time: %.2f ms\n"
    "  Peak memory usage: %llu MB\n"
    "  Average memory usage: %llu MB\n"
    "  Average queue size: %u frames\n"
    "  Peak queue size: %u frames\n"
    "  Hardware accelerated: %s\n",
    mStats.mTotalFramesDecoded,
    mStats.mTotalFramesDisplayed,
    mStats.mTotalFramesSkipped,
    mStats.mTotalFramesDropped,
    mStats.mAverageDecodeTimeMs,
    mStats.mMaxDecodeTimeMs,
    mStats.mPeakMemoryUsage / (1024 * 1024),
    mStats.mAverageMemoryUsage / (1024 * 1024),
    mStats.mAverageQueueSize,
    mStats.mPeakQueueSize,
    mStats.mIsHardwareAccelerated ? "yes" : "no");
  
  aOutput.AssignASCII(info.get());
}

} // namespace mozilla
