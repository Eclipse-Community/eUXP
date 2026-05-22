/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AsyncImageDecoder.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace image {

nsresult AsyncImageDecoder::DispatchToMainThread(nsIRunnable* aRunnable) {
  NS_ASSERTION(aRunnable != nullptr, "Runnable must not be null");

  return NS_DispatchToMainThread(aRunnable, NS_DISPATCH_NORMAL);
}

}  // namespace image
}  // namespace mozilla
