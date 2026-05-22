/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_layout_ParallelLayoutComputation_h
#define mozilla_layout_ParallelLayoutComputation_h

#include "mozilla/RefPtr.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/SharedThreadPool.h"
#include "nsThreadUtils.h"
#include "mozilla/UniquePtr.h"
#include "nsTArray.h"

class nsIFrame;
class nsPresContext;

namespace mozilla {
namespace layout {

/**
 * FrameComputationTask represents a layout computation task for a single frame
 * or subtree that can be executed on a worker thread.
 * 
 * Measurements, style recalculation, and pre-layout computations can often
 * be parallelized across independent branches of the frame tree.
 */
class FrameComputationTask : public Runnable {
 public:
  explicit FrameComputationTask(nsIFrame* aFrame)
  : Runnable(), mFrame(aFrame) {}

  NS_IMETHOD Run() override;

 protected:
  nsIFrame* mFrame;

  // Override to implement specific computation logic
  virtual nsresult ComputeFrame() = 0;
};

/**
 * LayoutWorkerPool manages parallel layout computations.
 * 
 * The layout system benefits from parallelizing independent computations:
 * - Style recalculation on independent subtrees
 * - Measurement passes for independent branches
 * - Pre-computation of layout hints
 * 
 * Usage:
 *   RefPtr<LayoutWorkerPool> pool = LayoutWorkerPool::Get();
 *   RefPtr<FrameComputationTask> task = new MyLayoutTask(frame);
 *   pool->Dispatch(task);
 *   pool->WaitForCompletion();
 */
class LayoutWorkerPool final : public RefCounted<LayoutWorkerPool> {
 public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(LayoutWorkerPool)

  ~LayoutWorkerPool() = default;

  /**
   * Get the global layout worker pool
   */
  static already_AddRefed<LayoutWorkerPool> Get();

  /**
   * Dispatch a layout computation task
   */
  nsresult Dispatch(FrameComputationTask* aTask);

  /**
   * Dispatch multiple independent frame tasks in parallel
   */
  nsresult DispatchFrameTree(nsIFrame* aRoot);

  /**
   * Wait for all pending tasks to complete
   */
  nsresult WaitForCompletion();

  /**
   * Get the underlying thread pool
   */
  SharedThreadPool* GetThreadPool() const { return mThreadPool; }

 private:
  LayoutWorkerPool() : mThreadPool(nullptr), mPendingTasks(0) {}

  RefPtr<SharedThreadPool> mThreadPool;
  int32_t mPendingTasks;

  friend class StaticAutoPtr<LayoutWorkerPool>;
};

/**
 * StyleRecalcTask - Parallel style recalculation task
 * 
 * Recalculates styles for a frame and its independent children on a worker
 * thread, then merges results back on the main thread.
 */
class StyleRecalcTask : public FrameComputationTask {
 public:
  explicit StyleRecalcTask(nsIFrame* aFrame)
      : FrameComputationTask(aFrame) {}

 protected:
  nsresult ComputeFrame() override;
};

/**
 * MeasureTask - Parallel measurement pass
 * 
 * Performs text measurement, content size calculations, and other
 * measurement operations that require heavy computation but have
 * limited dependencies.
 */
class MeasureTask : public FrameComputationTask {
 public:
  explicit MeasureTask(nsIFrame* aFrame) : FrameComputationTask(aFrame) {}

 protected:
  nsresult ComputeFrame() override;
};

/**
 * ReflowPreComputeTask - Pre-compute reflow information
 * 
 * Early computation of reflow hints, constraints, and available space
 * for independent frame branches.
 */
class ReflowPreComputeTask : public FrameComputationTask {
 public:
  explicit ReflowPreComputeTask(nsIFrame* aFrame)
      : FrameComputationTask(aFrame) {}

 protected:
  nsresult ComputeFrame() override;
};

}  // namespace layout
}  // namespace mozilla

#endif  // mozilla_layout_ParallelLayoutComputation_h
