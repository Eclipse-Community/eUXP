/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZILLA_ADAPTIVEBUFFERINGPOLICY_H_
#define MOZILLA_ADAPTIVEBUFFERINGPOLICY_H_

#include "mozilla/Atomics.h"
#include "TimeUnits.h"
#include "gfxTypes.h"

namespace mozilla {

/**
 * Adaptive buffering policy manager for video playback.
 * 
 * Dynamically adjusts buffering thresholds based on:
 * - Available system memory
 * - Video resolution and bitrate
 * - Decode speed vs real-time ratio
 * - Network conditions (if applicable)
 * 
 * This reduces memory usage while maintaining smooth playback.
 */
class AdaptiveBufferingPolicy {
public:
  struct Config {
    // Audio buffering thresholds (microseconds)
    uint64_t mLowAudioThreshold = 300000;     // 300ms - trigger faster decoding
    uint64_t mAmpleAudioThreshold = 2000000;  // 2s - target comfortable level
    
    // Video frame queue sizing
    uint32_t mMinVideoQueueFrames = 3;        // Absolute minimum
    uint32_t mDefaultVideoQueueFrames = 10;   // Default for standard content
    uint32_t mMaxVideoQueueFrames = 20;       // Absolute maximum
    
    // Hardware acceleration thresholds
    uint32_t mHWAccelVideoQueueFrames = 5;    // Target for HW-accelerated playback
    
    // Memory limits (bytes)
    uint64_t mMaxVideoBufferMemory = 100 * 1024 * 1024;    // 100MB default
    uint64_t mTargetVideoBufferMemory = 50 * 1024 * 1024;  // 50MB target
    
    // Decode speed thresholds
    double mDecodeSpeedRatio = 1.5;           // Must decode 1.5x realtime speed
    
    // Frame skip configuration
    bool mEnableAdaptiveFrameSkip = true;
    bool mSkipNonKeyframes = true;
    uint32_t mFrameSkipThresholdMs = 60;      // Skip if >60ms behind audio
  };

  /**
   * Adjust buffering policy based on system state and video properties.
   * Returns optimized configuration for the given video specifications.
   */
  static Config ComputeOptimalConfig(
    const gfx::IntSize& aVideoSize,
    uint32_t aFrameRate,
    uint64_t aVideoBitrate,
    bool aHardwareAccelerated,
    uint64_t aAvailableMemory);

  /**
   * Get the recommended video queue size based on context.
   */
  static uint32_t GetOptimalVideoQueueSize(
    const gfx::IntSize& aVideoSize,
    bool aHardwareAccelerated,
    uint64_t aAvailableMemory);

  /**
   * Get audio buffering threshold based on network conditions.
   */
  static uint64_t GetOptimalAudioThreshold(bool aIsNetworkStream);

  /**
   * For HD+ content with limited memory, suggest lower priority video frames
   * that can be safely skipped without visible quality loss.
   */
  static bool ShouldSkipFrame(
    int64_t aAudioTimeUs,
    int64_t aFrameDisplayTimeUs,
    int64_t aVideoDecodeTimeUs,
    uint32_t aThresholdMs);
};

} // namespace mozilla

#endif // MOZILLA_ADAPTIVEBUFFERINGPOLICY_H_
