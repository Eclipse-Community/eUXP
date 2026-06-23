/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AdaptiveBufferingPolicy.h"
#include "mozilla/Logging.h"

namespace mozilla {

extern LazyLogModule gMediaDecoderLog;

#define LOG(arg, ...) MOZ_LOG(gMediaDecoderLog, mozilla::LogLevel::Debug, ("AdaptiveBuffering::%s: " arg, __func__, ##__VA_ARGS__))

/**
 * Compute estimated memory usage for a single frame.
 */
static uint64_t EstimateFrameMemory(const gfx::IntSize& aSize)
{
  // YUV420 format: 1.5 bytes per pixel
  uint64_t width = aSize.width;
  uint64_t height = aSize.height;
  return (width * height * 3) / 2;
}

/**
 * Calculate resolution tier (affects quality and frame size).
 */
static uint32_t GetResolutionTier(const gfx::IntSize& aSize)
{
  uint32_t pixels = aSize.width * aSize.height;
  
  // Classify resolution
  if (pixels < 480 * 360) return 0;          // SD-like
  if (pixels < 1280 * 720) return 1;         // HD-ready
  if (pixels < 1920 * 1080) return 2;        // Full HD
  if (pixels < 3840 * 2160) return 3;        // 4K
  return 4;                                  // 8K+
}

AdaptiveBufferingPolicy::Config 
AdaptiveBufferingPolicy::ComputeOptimalConfig(
    const gfx::IntSize& aVideoSize,
    uint32_t aFrameRate,
    uint64_t aVideoBitrate,
    bool aHardwareAccelerated,
    uint64_t aAvailableMemory)
{
  Config config;
  uint32_t resolutionTier = GetResolutionTier(aVideoSize);
  uint64_t frameMemory = EstimateFrameMemory(aVideoSize);
  
  // Adjust video queue size based on resolution and memory availability
  uint32_t recommendedQueueSize = config.mDefaultVideoQueueFrames;
  
  if (aHardwareAccelerated) {
    recommendedQueueSize = config.mHWAccelVideoQueueFrames;
  }
  
  // For high-resolution content, reduce queue size to save memory
  if (resolutionTier >= 3) {  // 4K or higher
    recommendedQueueSize = std::max(config.mMinVideoQueueFrames, 
                                    recommendedQueueSize / 2);
    config.mMaxVideoBufferMemory = 50 * 1024 * 1024;  // 50MB for 4K
  } else if (resolutionTier == 2) {  // Full HD
    config.mMaxVideoBufferMemory = 75 * 1024 * 1024;  // 75MB
  }
  
  // Constrain by available memory
  uint64_t maxFramesInMemory = config.mMaxVideoBufferMemory / 
                               std::max(frameMemory, (uint64_t)1);
  recommendedQueueSize = std::min((uint32_t)maxFramesInMemory, recommendedQueueSize);
  recommendedQueueSize = std::max(config.mMinVideoQueueFrames, recommendedQueueSize);
  
  config.mDefaultVideoQueueFrames = recommendedQueueSize;
  
  // Adjust decode speed threshold based on bitrate
  if (aVideoBitrate > 0) {
    // High bitrate content may need faster decode speed
    if (aVideoBitrate > 10000000) {  // >10 Mbps
      config.mDecodeSpeedRatio = 2.0;
    } else if (aVideoBitrate > 5000000) {  // >5 Mbps
      config.mDecodeSpeedRatio = 1.75;
    }
  }
  
  // For network streams, use more conservative buffering
  if (aVideoBitrate > 0) {
    config.mLowAudioThreshold = 500000;     // 500ms
    config.mAmpleAudioThreshold = 3000000;  // 3s
  }
  
  LOG("Computed config: queueSize=%u, maxMemory=%llu, decodeRatio=%.2f (res=%u)",
      recommendedQueueSize,
      config.mMaxVideoBufferMemory,
      config.mDecodeSpeedRatio,
      resolutionTier);
  
  return config;
}

uint32_t AdaptiveBufferingPolicy::GetOptimalVideoQueueSize(
    const gfx::IntSize& aVideoSize,
    bool aHardwareAccelerated,
    uint64_t aAvailableMemory)
{
  // Start with base size
  uint32_t queueSize = aHardwareAccelerated ? 5 : 10;
  
  uint64_t frameMemory = EstimateFrameMemory(aVideoSize);
  uint64_t maxFramesInMemory = (100 * 1024 * 1024) / frameMemory;
  
  // Constrain by available memory
  queueSize = std::min((uint32_t)maxFramesInMemory, queueSize);
  
  return std::max(3u, queueSize);
}

uint64_t AdaptiveBufferingPolicy::GetOptimalAudioThreshold(bool aIsNetworkStream)
{
  if (aIsNetworkStream) {
    return 500000;  // 500ms for network (more conservative)
  }
  return 300000;    // 300ms for local files
}

bool AdaptiveBufferingPolicy::ShouldSkipFrame(
    int64_t aAudioTimeUs,
    int64_t aFrameDisplayTimeUs,
    int64_t aVideoDecodeTimeUs,
    uint32_t aThresholdMs)
{
  // Skip if frame is already in the past relative to audio
  int64_t thresholdUs = aThresholdMs * 1000;
  
  // Calculate how late this frame is
  int64_t timeLatenessUs = aAudioTimeUs - aFrameDisplayTimeUs;
  
  // Skip if frame is significantly behind audio timeline
  if (timeLatenessUs > thresholdUs) {
    return true;
  }
  
  // Also skip if decoding this frame would take us further behind
  int64_t projectedLatenessUs = timeLatenessUs + aVideoDecodeTimeUs;
  if (projectedLatenessUs > (thresholdUs * 2)) {
    return true;
  }
  
  return false;
}

} // namespace mozilla
