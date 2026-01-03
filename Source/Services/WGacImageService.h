#ifndef WGAC_IMAGESERVICE_H
#define WGAC_IMAGESERVICE_H

#include "GacUI.h"
#include <cairo/cairo.h>

namespace vl {
namespace presentation {
namespace wayland {

class WGacImageFrame : public Object, public INativeImageFrame
{
protected:
    cairo_surface_t* surface;
    INativeImage* image;
    Size size;
    collections::Dictionary<void*, Ptr<INativeImageFrameCache>> caches;

public:
    WGacImageFrame(INativeImage* image, cairo_surface_t* surface);
    ~WGacImageFrame();

    INativeImage* GetImage() override;
    Size GetSize() override;
    bool SetCache(void* key, Ptr<INativeImageFrameCache> cache) override;
    Ptr<INativeImageFrameCache> GetCache(void* key) override;
    Ptr<INativeImageFrameCache> RemoveCache(void* key) override;

    cairo_surface_t* GetSurface() { return surface; }
};

class WGacImage : public Object, public INativeImage
{
protected:
    INativeImageService* imageService;
    collections::Array<Ptr<WGacImageFrame>> frames;
    FormatType formatType;

public:
    WGacImage(INativeImageService* service, cairo_surface_t* surface);
    ~WGacImage();

    INativeImageService* GetImageService() override;
    FormatType GetFormat() override;
    vint GetFrameCount() override;
    INativeImageFrame* GetFrame(vint index) override;
    void SaveToStream(stream::IStream& stream, FormatType formatType) override;
};

class WGacImageService : public Object, public INativeImageService
{
public:
    Ptr<INativeImage> CreateImageFromFile(const WString& path) override;
    Ptr<INativeImage> CreateImageFromMemory(void* buffer, vint length) override;
    Ptr<INativeImage> CreateImageFromStream(stream::IStream& stream) override;
};

}
}
}

#endif // WGAC_IMAGESERVICE_H
