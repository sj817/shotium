/*
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkImageAndroid_DEFINED
#define SkImageAndroid_DEFINED

#include "include/core/SkImage.h"
#include "include/core/SkRefCnt.h"

namespace SkImages {

/**
 * Like SkImage::MakeFromBitmap, except and Skia will *not* copy the bitmap.
 */
SK_API sk_sp<SkImage> RasterFromBitmapNoCopy(const SkBitmap&);

}  // namespace SkImages

#endif
