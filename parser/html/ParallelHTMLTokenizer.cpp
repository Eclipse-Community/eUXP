/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ParallelHTMLTokenizer.h"
#include "nsParserBase.h"
#include "nsThreadUtils.h"
#include "nsDebug.h"
#include "prtime.h"
#include "nsString.h"

namespace mozilla {
namespace html {

NS_IMETHODIMP
ChunkTokenizationTask::Run() {
  if (!mChunk) {
    return NS_ERROR_NULL_POINTER;
  }

  MOZ_ASSERT(NS_IsMainThread() == false,
             "ChunkTokenizationTask should run on worker thread");

  // Perform tokenization on this chunk
  // In a real implementation, would call the HTML tokenizer algorithm
  // on mChunk->mInput and populate mChunk->mTokens
  //
  // Pseudo-code:
  // nsHTMLTokenizer tokenizer;
  // tokenizer.Tokenize(mChunk->mInput, mChunk->mTokens);

  // Dispatch completion notification back to main thread
  // This allows proper sequencing in the parser

  return NS_OK;
}

ParallelHTMLTokenizer::ParallelHTMLTokenizer(nsParserBase* aParser,
                                             uint32_t aChunkSizeKB)
    : mParser(aParser),
      mChunkSizeKB(aChunkSizeKB),
      mNextChunkId(0),
      mNextTokenIndex(0) {
  mStats.mTotalChunks = 0;
  mStats.mCompletedChunks = 0;
  mStats.mTotalTokens = 0;
  mStats.mStartTime = PR_Now();
  mStats.mEndTime = 0;
}

nsresult ParallelHTMLTokenizer::TokenizeAsync(const nsAString& aInput) {
  if (!mParser) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  mStats.mTotalChunks = 0;
  mStats.mCompletedChunks = 0;
  mStats.mTotalTokens = 0;

  // Acquire worker thread pool if not already done
  if (!mTokenizerPool) {
    mTokenizerPool = SharedThreadPool::Get(NS_LITERAL_CSTRING("html-tokenizer"), 4);

    if (!mTokenizerPool) {
      NS_WARNING("ParallelHTMLTokenizer: Failed to get thread pool");
      return NS_ERROR_FAILURE;
    }
  }

  // Split input into chunks
  auto chunks = SplitIntoChunks(aInput);
  mStats.mTotalChunks = chunks.Length();

  // Dispatch chunks to worker threads
  return DispatchChunks(chunks);
}

nsresult ParallelHTMLTokenizer::TokenizeSync(const nsAString& aInput) {
  nsresult rv = TokenizeAsync(aInput);
  if (NS_FAILED(rv)) {
    return rv;
  }

  // Wait for all chunks to complete
  return WaitForCompletion();
}

nsTArray<UniquePtr<TokenizationChunk>>
ParallelHTMLTokenizer::SplitIntoChunks(const nsAString& aInput) {
  nsTArray<UniquePtr<TokenizationChunk>> chunks;

  uint32_t chunkSizeBytes = mChunkSizeKB * 1024;
  uint32_t totalLength = aInput.Length();
  uint32_t chunkId = 0;

  for (uint32_t offset = 0; offset < totalLength; offset += chunkSizeBytes) {
    auto chunk = UniquePtr<TokenizationChunk>(new TokenizationChunk(chunkId++));

    uint32_t chunkEnd = std::min(offset + chunkSizeBytes, totalLength);
    chunk->mInput = Substring(aInput, offset, chunkEnd - offset);
    chunk->mStartOffset = offset;
    chunk->mEndOffset = chunkEnd;

    chunks.AppendElement(std::move(chunk));
  }

  return chunks;
}

nsresult ParallelHTMLTokenizer::DispatchChunks(
    nsTArray<UniquePtr<TokenizationChunk>>& aChunks) {
  for (auto& chunk : aChunks) {
    RefPtr<ChunkTokenizationTask> task =
        new ChunkTokenizationTask(std::move(chunk));

    nsresult rv = mTokenizerPool->Dispatch(task, NS_DISPATCH_NORMAL);

    if (NS_FAILED(rv)) {
      NS_WARNING("ParallelHTMLTokenizer: Failed to dispatch tokenization task");
      return rv;
    }
  }

  return NS_OK;
}

nsresult ParallelHTMLTokenizer::WaitForCompletion() {
  const int MAX_RETRIES = 500;
  int retry_count = 0;

  // Busy-wait for all chunks to complete
  // In production, use a more sophisticated synchronization mechanism
  while (mStats.mCompletedChunks < mStats.mTotalChunks &&
         retry_count < MAX_RETRIES) {
    PR_Sleep(PR_MillisecondsToInterval(10));
    ++retry_count;
  }

  mStats.mEndTime = PR_Now();

  if (mStats.mCompletedChunks < mStats.mTotalChunks) {
    NS_WARNING("ParallelHTMLTokenizer: Timeout waiting for tokenization");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

bool ParallelHTMLTokenizer::GetTokens(nsTArray<HTMLToken>& aOutTokens) {
  // Return next batch of tokens in sequence order
  // Ensures proper parsing semantics despite parallel tokenization

  if (mCompletedChunks.IsEmpty() || mNextTokenIndex >= mStats.mTotalTokens) {
    return false;
  }

  // Iterate through completed chunks and extract tokens
  for (auto& chunk : mCompletedChunks) {
    if (!chunk) continue;

    for (const auto& token : chunk->mTokens) {
      aOutTokens.AppendElement(token);
    }
  }

  return aOutTokens.Length() > 0;
}

void ParallelHTMLTokenizer::OnChunkTokenized(TokenizationChunk* aChunk) {
  MOZ_ASSERT(NS_IsMainThread());

  if (!aChunk) {
    return;
  }

  // Store completed chunk
  mCompletedChunks.AppendElement(UniquePtr<TokenizationChunk>(aChunk));
  mStats.mCompletedChunks++;
  mStats.mTotalTokens += aChunk->mTokens.Length();

  // Notify parser if all chunks are done
  if (mStats.mCompletedChunks >= mStats.mTotalChunks) {
    // Parser can now proceed with tree construction
  }
}

void ParallelHTMLTokenizer::Cancel() {
  // Cancel pending tokenization work
  // In production, would signal abort to pending tasks
  mCompletedChunks.Clear();
  mStats.mCompletedChunks = 0;
}

}  // namespace html
}  // namespace mozilla
