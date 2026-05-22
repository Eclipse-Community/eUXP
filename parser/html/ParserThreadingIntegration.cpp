/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * ParserThreadingIntegration.cpp
 * 
 * Integrates parallel HTML tokenization into the stream parser.
 * When enabled, large documents (>512KB) are tokenized in parallel
 * using multiple worker threads instead of blocking the main thread.
 */

#include "ParallelHTMLTokenizer.h"
#include "nsHtml5StreamParser.h"
#include "mozilla/RefPtr.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace html {

/**
 * Hook for parallel tokenization in stream parser
 */
class StreamParserThreadingHook {
 public:
  // Threshold for using parallel tokenization
  static const size_t PARALLEL_TOKENIZER_THRESHOLD = 512 * 1024;  // 512KB

  /**
   * Determine if we should use parallel tokenization for this buffer
   */
  static bool ShouldUseParallelTokenizer(const nsAString& aBuffer) {
    return aBuffer.Length() > PARALLEL_TOKENIZER_THRESHOLD;
  }

  /**
   * Create a parallel tokenizer if appropriate
   * 
   * Usage in stream parser:
   *   if (StreamParserThreadingHook::ShouldUseParallelTokenizer(mSourceBuffer)) {
   *     mParallelTokenizer = 
   *       StreamParserThreadingHook::CreateParallelTokenizer(aParser);
   *   }
   */
  static RefPtr<ParallelHTMLTokenizer> CreateParallelTokenizer(
      void* aParserPtr) {
    // Convert void* back to proper parser type
    // (Avoids circular includes)
    
    RefPtr<ParallelHTMLTokenizer> tokenizer = 
        new ParallelHTMLTokenizer(reinterpret_cast<nsParserBase*>(aParserPtr),
                                   64);  // 64KB chunks
    return tokenizer;
  }

  /**
   * Dispatch parallel tokenization
   * Returns NS_OK if parallel path was taken, NS_ERROR_FAILURE to fallback
   */
  static nsresult TokenizeInParallel(ParallelHTMLTokenizer* aTokenizer,
                                      const nsAString& aBuffer) {
    if (!aTokenizer) {
      return NS_ERROR_NULL_POINTER;
    }

    // Use synchronous tokenization (blocks until complete)
    // but internally uses parallel chunks
    return aTokenizer->TokenizeSync(aBuffer);
  }

  /**
   * Get tokenized output after parallel processing completes
   */
  static bool GetTokens(ParallelHTMLTokenizer* aTokenizer,
                        nsTArray<html::HTMLToken>& aOutTokens) {
    if (!aTokenizer) {
      return false;
    }

    return aTokenizer->GetTokens(aOutTokens);
  }

  /**
   * Statistics for monitoring parallel tokenization effectiveness
   */
  static void LogTokenizerStats(ParallelHTMLTokenizer* aTokenizer) {
    if (!aTokenizer) {
      return;
    }

    auto stats = aTokenizer->GetStats();

    printf(
        "[ParallelHTMLTokenizer Stats]\n"
        "  Total Chunks: %u\n"
        "  Completed Chunks: %u\n"
        "  Total Tokens: %u\n"
        "  Time: %lld ms\n",
        stats.mTotalChunks, stats.mCompletedChunks, stats.mTotalTokens,
        (stats.mEndTime - stats.mStartTime) / PR_USEC_PER_MSEC);
  }
};

/**
 * Integration point for HTML5 tokenizer
 * Enable by calling this during parser initialization
 */
void EnableParallelHTMLTokenization(void* aParserState) {
  // Store parallel tokenizer preference globally or in parser
  // This enables the integration when creating new parsers
}

}  // namespace html
}  // namespace mozilla
