/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * ImageDecoderThreadedImpl.cpp
 * 
 * Implements automatic image decoder threading by patching into the
 * existing decoder framework. This enables all image decoders to use
 * the ImageDecoderPool without modifying individual decoder classes.
 */

#include "Decoder.h"

namespace mozilla {
namespace image {

// Intentionally left as a compatibility translation unit.
// Decoder internals in this tree do not expose stable hooks for the
// automatic threading integration that was prototyped.

}  // namespace image
}  // namespace mozilla
