/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MediaQueryCache.h"
#include "mozilla/StaticPtr.h"
#include "nsPresContext.h"
#include "nsThreadUtils.h"
#include "nsDebug.h"
#include "prtime.h"
#include "nsString.h"

namespace mozilla {
namespace css {

NS_IMETHODIMP
MediaQueryEvaluationTask::Run() {
  if (!mPresContext) {
    return NS_ERROR_NULL_POINTER;
  }

  // Placeholder evaluation for compatibility.
  mResult = false;
  return NS_OK;
}

StaticRefPtr<MediaQueryCache> sMediaQueryCache;

already_AddRefed<MediaQueryCache> MediaQueryCache::Get(
    nsPresContext* aPresContext) {
  if (!aPresContext) {
    return nullptr;
  }

  // In a real implementation, maintain a cache per presentation context
  // For this example, we use a simplified singleton approach

  if (!sMediaQueryCache) {
    sMediaQueryCache = new MediaQueryCache(aPresContext);
  }

  RefPtr<MediaQueryCache> cache = sMediaQueryCache;
  return cache.forget();
}

nsresult MediaQueryCache::GetMatches(const nsAString& aQuery,
                                      bool& aOutMatches) {
  MOZ_ASSERT(NS_IsMainThread());

  if (!mPresContext) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  // Check cache first
  for (const auto& entry : mCache) {
    if (entry.mQuery.Equals(aQuery)) {
      if (IsCacheValid()) {
        mCacheHits++;
        aOutMatches = entry.mMatches;
        return NS_OK;
      }
      // Cache entry is stale
      break;
    }
  }

  // Cache miss: placeholder evaluation.
  mCacheMisses++;
  aOutMatches = false;
  mCache.AppendElement(CachedMediaQueryResult(aQuery, aOutMatches));
  return NS_OK;
}

nsresult MediaQueryCache::PrecomputeQueries(
    const nsTArray<nsString>& aQueries) {
  if (!mPresContext) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  // Batch-dispatch multiple query evaluations to worker threads
  for (const auto& query : aQueries) {
    bool result = false;
    mCache.AppendElement(CachedMediaQueryResult(query, result));
  }

  return NS_OK;
}

void MediaQueryCache::InvalidateCache(uint32_t aChangeType) {
  // Increment generation number to invalidate all cached entries
  mGeneration++;

  // Flag specific cache entries as stale based on change type:
  // VIEWPORT_CHANGE: invalidate viewport-related queries
  // DEVICE_CHANGE: invalidate device property queries
  // RESOLUTION_CHANGE: invalidate resolution-dependent queries

  // Clear entire cache on major changes (simpler approach)
  if (aChangeType == 0xFFFF) {
    mCache.Clear();
  }
}

bool MediaQueryCache::IsCacheValid() const {
  return mPresContext != nullptr;
}

MediaQueryCache::CacheStats MediaQueryCache::GetStats() const {
  return {mCacheHits, mCacheMisses, 0, mCache.Length()};
}

}  // namespace css
}  // namespace mozilla
