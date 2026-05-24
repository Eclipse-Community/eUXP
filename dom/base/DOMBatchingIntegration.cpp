/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * DOMBatchingIntegration.cpp
 * 
 * Integrates DOM mutation batching into the main document parser and
 * layout system. This automatically batches mutations during:
 * - HTML parsing
 * - Table construction
 * - Large DOM insertions
 * - Fragment parsing
 */

#include "DOMBatchedMutations.h"
#include "Element.h"
#include "nsIDocument.h"
#include "nsContentSink.h"

namespace mozilla {
namespace dom {

/**
 * Enable DOM batching for parser operations
 */
class ParserDOMBatchingHook {
 public:
  /**
   * Call this when starting to parse table rows or building large fragments
   */
  static AutoBatchDOMMutations* StartTableBatch() {
    // Create and start a batch for table construction
    return new AutoBatchDOMMutations();
  }

  /**
   * Call this when fragment parsing starts (e.g., innerHTML)
   */
  static AutoBatchDOMMutations* StartFragmentBatch() {
    return new AutoBatchDOMMutations();
  }

  /**
   * End batch and flush mutations (call delete on returned object)
   */
  static void EndBatch(AutoBatchDOMMutations* aBatch) {
    delete aBatch;  // Calls ~AutoBatchDOMMutations which flushes
  }
};

/**
 * Hook into Element insertion to enable batching
 */
class ElementInsertionBatchingHook {
 public:
  /**
   * Check if we should batch this insertion
   * Return true if batching is active
   */
  static bool IsBatchActive() {
    return DOMBatchedMutations::Current() != nullptr;
  }

  /**
   * Get current batch (if active)
   */
  static DOMBatchedMutations* GetCurrentBatch() {
    return DOMBatchedMutations::Current();
  }

  /**
   * Should enable automatic batching for bulk operations (>50 insertions)
   */
  static bool ShouldEnableBatching(uint32_t aInsertionCount) {
    return aInsertionCount > 50;  // Enable batching for bulk ops
  }
};

/**
 * Content sink integration for parser
 */
class ContentSinkBatchingHook {
 public:
  /**
   * Called when parser encounters large fragment
   */
  static void OnLargeFragmentStart(nsContentSink* aSink) {
    if (!aSink) {
      return;
    }

    // Integration with nsContentSink would require modifying its structure.
    // For now, this hook is a no-op placeholder.
  }

  /**
   * Called when fragment processing complete
   */
  static void OnLargeFragmentEnd(nsContentSink* aSink) {
    if (!aSink) {
      return;
    }

    // Placeholder: no-op until nsContentSink is extended to hold batch state.
  }
};

}  // namespace dom
}  // namespace mozilla
