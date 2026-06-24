/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* Helper to get a dedicated single-thread pool for browser UI-sensitive work.
 * This pool is intentionally small (1 thread) so UI tasks aren't starved by
 * other background work that shares larger pools.
 */
#ifndef BrowserUIThread_h_
#define BrowserUIThread_h_

#include "SharedThreadPool.h"
#include "nsString.h"

namespace mozilla {

static inline already_AddRefed<SharedThreadPool> GetBrowserUIThreadPool()
{
  return SharedThreadPool::Get(NS_LITERAL_CSTRING("browser-ui"), 1);
}

} // namespace mozilla

#endif // BrowserUIThread_h_
