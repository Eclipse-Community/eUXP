/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_image_AsyncImageDecoder_h
#define mozilla_image_AsyncImageDecoder_h

#include "nsThreadUtils.h"
#include "mozilla/RefPtr.h"
#include "mozilla/SharedThreadPool.h"
#include "nsCOMPtr.h"
#include "nsIRunnable.h"

namespace mozilla {
namespace image {

class Decoder;

/**
 * AsyncImageDecoder provides a base class for asynchronous image decoding tasks.
 * 
 * Subclasses implement the decoding logic in the Run() method which executes
 * on a separate thread from the thread pool. Results are dispatched back to the
 * main thread via callbacks.
 * 
 * Usage:
 *   RefPtr<MyDecodeTask> task = new MyDecodeTask(decoder, listener);
 *   RefPtr<SharedThreadPool> pool = ImageDecoderPool::GetDecoderPool();
 *   pool->Dispatch(task, NS_DISPATCH_NORMAL);
 */
class AsyncImageDecoder : public Runnable {
 public:
  explicit AsyncImageDecoder(Decoder* aDecoder)
  : Runnable(), mDecoder(aDecoder) {}

 protected:
  // Subclasses override this to perform the actual decoding on worker thread
  NS_IMETHOD Run() override = 0;

  /**
   * Helper: Dispatch a runnable to the main thread after decoding completes.
   * Used to notify the Decoder of completion and handle results.
   */
  static nsresult DispatchToMainThread(nsIRunnable* aRunnable);

  RefPtr<Decoder> mDecoder;
};

}  // namespace image
}  // namespace mozilla

#endif  // mozilla_image_AsyncImageDecoder_h
