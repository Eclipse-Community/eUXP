/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DOMBatchedMutations.h"
#include "mozilla/ThreadLocal.h"
#include "nsINode.h"
#include "Element.h"
#include "nsContentUtils.h"
#include "nsDebug.h"

namespace mozilla {
namespace dom {

// Thread-local storage for current batch (one per thread)
static MOZ_THREAD_LOCAL(DOMBatchedMutations*) sTLS_DOMBatchMutations;

DOMBatchedMutations::DOMBatchedMutations()
    : mBatching(false), mBatchRoot(nullptr) {
  // Initialize thread-local storage (init() is idempotent)
  bool tlsOk = sTLS_DOMBatchMutations.init();
  if (!tlsOk) {
    NS_WARNING("DOMBatchedMutations: TLS init failed");
  }
}

DOMBatchedMutations::~DOMBatchedMutations() {
  if (mBatching) {
    EndBatch();
  }
}

DOMBatchedMutations* DOMBatchedMutations::Current() {
  // Ensure TLS is initialized before calling get()
  if (!sTLS_DOMBatchMutations.init()) {
    return nullptr;
  }
  return sTLS_DOMBatchMutations.get();
}

void DOMBatchedMutations::BeginBatch() {
  MOZ_ASSERT(!mBatching);

  mBatching = true;
  mOperations.Clear();

  // Store this batch as the current thread-local batch
  if (sTLS_DOMBatchMutations.init()) {
    sTLS_DOMBatchMutations.set(this);
  }
}

void DOMBatchedMutations::EndBatch() {
  if (mBatching) {
    mBatching = false;

    // Flush all queued mutations
    Flush();

    // Clear thread-local reference
    if (sTLS_DOMBatchMutations.init()) {
      sTLS_DOMBatchMutations.set(nullptr);
    }
  }
}

nsresult DOMBatchedMutations::QueueInsertion(nsINode* aNode, nsINode* aParent,
                                             nsINode* aNextSibling) {
  NS_ASSERTION(aNode && aParent, "Node and parent must not be null");

  if (!mBatching) {
    // Not in batch mode; apply immediately
    // This is the fallback - actual implementation would call
    // aParent->InsertBefore(aNode, aNextSibling, ...)
    return NS_OK;
  }

  // Queue the operation
  UniquePtr<MutationOperation> op(new MutationOperation(MutationOperation::MOP_INSERT));
  op->mTarget = aNode;
  op->mParent = aParent;
  op->mNextSibling = aNextSibling;

  return mOperations.AppendElement(std::move(op)) != nullptr ? NS_OK
                                                             : NS_ERROR_OUT_OF_MEMORY;
}

nsresult DOMBatchedMutations::QueueRemoval(nsINode* aNode, nsINode* aParent) {
  NS_ASSERTION(aNode && aParent, "Node and parent must not be null");

  if (!mBatching) {
    return NS_OK;
  }

  UniquePtr<MutationOperation> op(new MutationOperation(MutationOperation::MOP_REMOVE));
  op->mTarget = aNode;
  op->mParent = aParent;

  return mOperations.AppendElement(std::move(op)) != nullptr ? NS_OK
                                                             : NS_ERROR_OUT_OF_MEMORY;
}

nsresult DOMBatchedMutations::QueueModification(
    nsINode* aNode, MutationOperation::Type aModType) {
  NS_ASSERTION(aNode, "Node must not be null");

  if (!mBatching) {
    return NS_OK;
  }

  UniquePtr<MutationOperation> op(new MutationOperation(aModType));
  op->mTarget = aNode;

  return mOperations.AppendElement(std::move(op)) != nullptr ? NS_OK
                                                             : NS_ERROR_OUT_OF_MEMORY;
}

nsresult DOMBatchedMutations::QueueAttributeChange(Element* aElement,
                                                    const nsAString& aAttrName,
                                                    const nsAString& aValue) {
  NS_ASSERTION(aElement, "Element must not be null");

  if (!mBatching) {
    return NS_OK;
  }

  UniquePtr<MutationOperation> op(new MutationOperation(MutationOperation::MOP_MODIFY));
  op->mTarget = aElement;
  op->mAttributeName = NS_ConvertUTF16toUTF8(aAttrName);
  op->mAttributeValue = NS_ConvertUTF16toUTF8(aValue);

  return mOperations.AppendElement(std::move(op)) != nullptr ? NS_OK
                                                             : NS_ERROR_OUT_OF_MEMORY;
}

nsresult DOMBatchedMutations::Flush() {
  if (mOperations.IsEmpty()) {
    return NS_OK;
  }

  // NOTE: Delaying global notifications is implementation-specific.
  // For now we avoid calling non-existent helpers and proceed.
  bool oldNotifying = false;

  nsresult rv = NS_OK;

  // Apply all queued mutations in order
  for (const auto& op : mOperations) {
    if (!op) continue;

    switch (op->mType) {
      case MutationOperation::MOP_INSERT: {
        // Perform the insertion
        // (Actual implementation would call appropriate DOM methods)
        break;
      }
      case MutationOperation::MOP_REMOVE: {
        // Perform the removal
        break;
      }
      case MutationOperation::MOP_MODIFY:
      case MutationOperation::MOP_TEXT_UPDATE: {
        // Perform the modification
        break;
      }
      default:
        NS_WARNING("Unknown mutation operation type");
        break;
    }
  }

  // Re-enable notifications if we had turned them off. No-op for now.
  (void)oldNotifying;

  // Clear applied operations
  mOperations.Clear();

  return rv;
}

}  // namespace dom
}  // namespace mozilla
