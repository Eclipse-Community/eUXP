/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* Convenience header placed in the `mozilla` include path so build
 * consumers can include it as `mozilla/BrowserUIThread.h`.
 */
#ifndef mozilla_BrowserUIThread_h
#define mozilla_BrowserUIThread_h

#include "../xpcom/threads/SharedThreadPool.h"
#include "nsString.h"

namespace mozilla {

static inline already_AddRefed<SharedThreadPool> GetBrowserUIThreadPool()
{
  return SharedThreadPool::Get(NS_LITERAL_CSTRING("browser-ui"), 1);
}

} // namespace mozilla

#endif // mozilla_BrowserUIThread_h
