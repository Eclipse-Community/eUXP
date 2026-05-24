/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_image_ImageDecoderPool_h
#define mozilla_image_ImageDecoderPool_h

#include "mozilla/SharedThreadPool.h"
#include "mozilla/RefPtr.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace image {

/**
 * ImageDecoderPool manages a shared thread pool for image decoding operations.
 * 
 * This offloads image decoding from the main thread, preventing UI jank from
 * heavy image processing operations. The pool maintains a configurable number
 * of worker threads (default: 4) and automatically shuts down when the last
 * reference is released.
 */
class ImageDecoderPool final {
 public:
  /**
   * Returns the global image decoder thread pool.
   * Uses lazy initialization - the pool is created on first access.
   * 
   * @param aMaxThreads Optional limit on number of concurrent decode threads
   * @return Already-AddRefed<SharedThreadPool> ready for dispatching work
   */
  static already_AddRefed<SharedThreadPool> GetDecoderPool(
      uint32_t aMaxThreads = 4);

  // Explicit deletion of copy operations (thread pool is singleton)
  ImageDecoderPool(const ImageDecoderPool&) = delete;
  void operator=(const ImageDecoderPool&) = delete;

 private:
  friend class StaticAutoPtr<ImageDecoderPool>;

  ImageDecoderPool() = default;
  ~ImageDecoderPool() = default;

  static StaticRefPtr<SharedThreadPool> sDecoderPool;
};

}  // namespace image
}  // namespace mozilla

#endif  // mozilla_image_ImageDecoderPool_h
