/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ImageDecoderPool.h"
#include "mozilla/StaticPtr.h"
#include "nsDebug.h"
#include "nsString.h"

namespace mozilla {
namespace image {

StaticRefPtr<SharedThreadPool> ImageDecoderPool::sDecoderPool;

already_AddRefed<SharedThreadPool> ImageDecoderPool::GetDecoderPool(
    uint32_t aMaxThreads) {
  // Lazy initialization pattern: Create pool only when first accessed
  if (!sDecoderPool) {
    // "image-decode" is the pool identifier used for naming threads
    // and coordinating with other subsystems
    sDecoderPool = SharedThreadPool::Get(NS_LITERAL_CSTRING("image-decode"),
                       aMaxThreads);

    // Verify pool was successfully created
    MOZ_ASSERT(sDecoderPool, "Failed to create image decoder thread pool");

    if (!sDecoderPool) {
      NS_WARNING(
          "ImageDecoderPool: Failed to create SharedThreadPool, image "
          "decoding will fall back to main thread");
    }
  }

  // Return a reference to the shared pool (thread-safe)
  RefPtr<SharedThreadPool> pool = sDecoderPool;
  return pool.forget();
}

}  // namespace image
}  // namespace mozilla
