/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_css_MediaQueryCache_h
#define mozilla_css_MediaQueryCache_h

#include "mozilla/RefPtr.h"
#include "mozilla/SharedThreadPool.h"
#include "nsThreadUtils.h"
#include "nsTArray.h"
#include "nsStringFwd.h"
#include "prtime.h"

class nsPresContext;

namespace mozilla {
namespace css {

class MediaQueryList;

/**
 * CachedMediaQueryResult stores the evaluation result of a media query
 * with associated metadata for cache invalidation.
 */
struct CachedMediaQueryResult {
  nsString mQuery;
  bool mMatches;
  uint64_t mTimestamp;  // When this result was computed
  uint32_t mGeneration; // Document generation when cached

  explicit CachedMediaQueryResult(const nsAString& aQuery, bool aMatches)
      : mQuery(aQuery),
        mMatches(aMatches),
        mTimestamp(PR_Now()),
        mGeneration(0) {}
};

/**
 * MediaQueryEvaluationTask performs expensive media query evaluation
 * asynchronously on a worker thread.
 * 
 * Media queries can be computationally expensive:
 * - Device property queries (orientation, resolution, color-depth)
 * - Viewport calculations
 * - Feature testing
 * - Complex boolean expressions
 * 
 * By evaluating on worker threads, we avoid blocking style recalculation.
 */
class MediaQueryEvaluationTask : public Runnable {
 public:
  MediaQueryEvaluationTask(const nsAString& aQuery, nsPresContext* aPresContext)
  : Runnable(),
        mQuery(aQuery),
        mPresContext(aPresContext),
        mResult(false) {}

  NS_IMETHOD Run() override;

  bool GetResult() const { return mResult; }

 protected:
  nsString mQuery;
  RefPtr<nsPresContext> mPresContext;
  bool mResult;
};

/**
 * MediaQueryCache provides high-performance caching and asynchronous
 * evaluation of CSS media queries.
 * 
 * Features:
 * - Result caching with generation tracking
 * - Async evaluation with callbacks
 * - Cache invalidation on viewport/device changes
 * - Batch pre-computation of common queries
 * 
 * Usage:
 *   RefPtr<MediaQueryCache> cache = MediaQueryCache::Get(presContext);
 *   cache->GetMatches("(orientation: landscape)")->Then(...);
 */
class MediaQueryCache final : public RefCounted<MediaQueryCache> {
 public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(MediaQueryCache)

  ~MediaQueryCache() = default;

  /**
   * Get the media query cache for a given presentation context
   */
  static already_AddRefed<MediaQueryCache> Get(nsPresContext* aPresContext);

  /**
   * Evaluate a media query asynchronously, returning a Promise.
   * Result is cached and all pending requests for the same query
   * are coalesced into a single evaluation.
   */
  nsresult GetMatches(const nsAString& aQuery, bool& aOutMatches);

  /**
   * Pre-compute evaluation results for a batch of common queries.
   * Useful for hot-path queries or during style recalculation.
   */
  nsresult PrecomputeQueries(const nsTArray<nsString>& aQueries);

  /**
   * Invalidate cached results due to viewport change, device change, etc.
   */
  void InvalidateCache(uint32_t aChangeType);

  /**
   * Check if a cached result is still valid
   */
  bool IsCacheValid() const;

  /**
   * Get cache statistics for debugging
   */
  struct CacheStats {
    uint32_t mHits;
    uint32_t mMisses;
    uint32_t mPendingEvaluations;
    size_t mCachedEntries;
  };

  CacheStats GetStats() const;

 private:
  friend class StaticAutoPtr<MediaQueryCache>;
  MediaQueryCache(nsPresContext* aPresContext) : mPresContext(aPresContext) {}

  RefPtr<nsPresContext> mPresContext;

  // Cache storage: query string -> result
  using CacheMap = nsTArray<CachedMediaQueryResult>;
  CacheMap mCache;

  // Generation number for invalidation tracking
  uint32_t mGeneration = 0;

  // Statistics
  uint32_t mCacheHits = 0;
  uint32_t mCacheMisses = 0;

  // Prevent copy/move
  MediaQueryCache(const MediaQueryCache&) = delete;
  MediaQueryCache& operator=(const MediaQueryCache&) = delete;
};

}  // namespace css
}  // namespace mozilla

#endif  // mozilla_css_MediaQueryCache_h
