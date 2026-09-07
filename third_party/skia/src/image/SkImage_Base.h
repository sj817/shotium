/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkImage_Base_DEFINED
#define SkImage_Base_DEFINED

#include "include/core/SkData.h"
#include "include/core/SkImage.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSpan.h"
#include "include/core/SkTypes.h"
#include "include/private/SkPixelStorage.h"
#include "include/private/SkTDArray.h"
#include "src/capture/SkCaptureManager.h"
#include "src/core/SkMipmap.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

class SkBitmap;
class SkColorSpace;
class SkPixmap;
class SkRecorder;
class SkSurface;
enum SkYUVColorSpace : int;
struct SkIRect;
struct SkISize;
struct SkImageInfo;

enum {
    kNeedNewImageUniqueID = 0
};

class SkImage_Base : public SkImage {
public:
    ~SkImage_Base() override;

    // From SkImage.h
    sk_sp<SkImage> makeColorSpace(SkRecorder*,
                                  sk_sp<SkColorSpace>,
                                  RequiredProperties) const override;

    sk_sp<SkImage> makeSubset(SkRecorder*, const SkIRect&, RequiredProperties) const override;

    // Methods that we want to use elsewhere in Skia, but not be a part of the public API.
    virtual bool onPeekPixels(SkPixmap*) const { return false; }

    virtual const SkBitmap* onPeekBitmap() const { return nullptr; }

    virtual bool onReadPixels(const SkImageInfo& dstInfo,
                              void* dstPixels,
                              size_t dstRowBytes,
                              int srcX,
                              int srcY,
                              CachingHint) const = 0;

    virtual bool readPixelsGraphite(SkRecorder*, const SkPixmap& dst, int srcX, int srcY) const {
        return false;
    }

    virtual bool onHasMipmaps() const = 0;
    virtual bool onIsProtected() const = 0;

    virtual SkMipmap* onPeekMips() const { return nullptr; }

    sk_sp<SkMipmap> refMips() const {
        return sk_ref_sp(this->onPeekMips());
    }

    /**
     * Default implementation does a rescale/read and then calls the callback.
     */
    virtual void onAsyncRescaleAndReadPixels(const SkImageInfo&,
                                             SkIRect srcRect,
                                             RescaleGamma,
                                             RescaleMode,
                                             ReadPixelsCallback,
                                             ReadPixelsContext) const;
    /**
     * Default implementation does a rescale/read/yuv conversion and then calls the callback.
     */
    virtual void onAsyncRescaleAndReadPixelsYUV420(SkYUVColorSpace,
                                                   bool readAlpha,
                                                   sk_sp<SkColorSpace> dstColorSpace,
                                                   SkIRect srcRect,
                                                   SkISize dstSize,
                                                   RescaleGamma,
                                                   RescaleMode,
                                                   ReadPixelsCallback,
                                                   ReadPixelsContext) const;



    // If this image is the current cached image snapshot of a surface then this is called when the
    // surface is destroyed to indicate no further writes may happen to surface backing store.
    virtual void generatingSurfaceIsDeleted() {}

    // return a read-only copy of the pixels. We promise to not modify them,
    // but only inspect them (or encode them).
    virtual bool getROPixels(SkBitmap*,
                             CachingHint = kAllow_CachingHint) const = 0;

    virtual sk_sp<SkImage> onMakeSubset(SkRecorder*, const SkIRect&, RequiredProperties) const = 0;

    virtual sk_sp<const SkData> onRefEncoded() const { return nullptr; }

    virtual bool onAsLegacyBitmap(SkBitmap*) const;

    // Create the surface used by makeScaled. If this is a GPU backed image, the surface
    // should be Ganesh or Graphite backed (as appropriate), otherwise this can raster backed.
    virtual sk_sp<SkSurface> onMakeSurface(SkRecorder*, const SkImageInfo&) const = 0;

    enum class Type {
        kRaster,
        kLazy,
        kLazyPicture,
    };

    virtual Type type() const = 0;

    // True for picture-backed and codec-backed
    bool isLazyGenerated() const override {
        return this->type() == Type::kLazy || this->type() == Type::kLazyPicture;
    }

    bool isRasterBacked() const {
        return this->type() == Type::kRaster;
    }

    // Call when this image is part of the key to a resourcecache entry. This allows the cache
    // to know automatically those entries can be purged when this SkImage deleted.
    virtual void notifyAddedToRasterCache() const {
        fAddedToRasterCache.store(true);
    }

    virtual sk_sp<SkImage> onReinterpretColorSpace(sk_sp<SkColorSpace>) const = 0;

    // on failure, returns nullptr
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    virtual sk_sp<SkImage> onMakeWithMipmaps(sk_sp<SkMipmap>) const {
        return nullptr;
    }

    SkSpan<const sk_sp<SkPixelStorage>> getPixelStorages() const {
        return fPixelStorages;
    }

protected:
    SkImage_Base(const SkImageInfo& info, uint32_t uniqueID, sk_sp<SkPixelStorage> backingStorage);
    // fPixelStorages is protected since SkImage_Raster needs access in ctor
    skia_private::STArray<1, sk_sp<SkPixelStorage>> fPixelStorages;
private:
    // Set true by caches when they cache content that's derived from the current pixels.

    mutable std::atomic<bool> fAddedToRasterCache;
};

static inline SkImage_Base* as_IB(SkImage* image) {
    return static_cast<SkImage_Base*>(image);
}

static inline SkImage_Base* as_IB(const sk_sp<SkImage>& image) {
    return static_cast<SkImage_Base*>(image.get());
}

static inline const SkImage_Base* as_IB(const SkImage* image) {
    return static_cast<const SkImage_Base*>(image);
}

static inline const SkImage_Base* as_IB(const sk_sp<const SkImage>& image) {
    return static_cast<const SkImage_Base*>(image.get());
}

#endif
