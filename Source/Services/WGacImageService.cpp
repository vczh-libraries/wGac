#include "WGacImageService.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

namespace vl {
namespace presentation {
namespace wayland {

// Helper function to copy pixbuf to cairo surface
static void copy_pixbuf_to_surface(GdkPixbuf* pixbuf, cairo_surface_t* surface)
{
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);

    unsigned char* dest = cairo_image_surface_get_data(surface);
    int dest_stride = cairo_image_surface_get_stride(surface);

    cairo_surface_flush(surface);

    for (int y = 0; y < height; y++) {
        guchar* src_row = pixels + y * rowstride;
        uint32_t* dest_row = (uint32_t*)(dest + y * dest_stride);

        for (int x = 0; x < width; x++) {
            guchar r = src_row[x * n_channels + 0];
            guchar g = src_row[x * n_channels + 1];
            guchar b = src_row[x * n_channels + 2];
            guchar a = (n_channels == 4) ? src_row[x * n_channels + 3] : 255;

            // Cairo uses premultiplied alpha
            r = r * a / 255;
            g = g * a / 255;
            b = b * a / 255;

            // ARGB32 format: 0xAARRGGBB
            dest_row[x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }

    cairo_surface_mark_dirty(surface);
}

// WGacImageFrame implementation
WGacImageFrame::WGacImageFrame(INativeImage* _image, cairo_surface_t* _surface)
    : image(_image)
    , surface(_surface)
{
    if (surface) {
        size = Size(
            cairo_image_surface_get_width(surface),
            cairo_image_surface_get_height(surface)
        );
    }
}

WGacImageFrame::~WGacImageFrame()
{
    if (surface) {
        cairo_surface_destroy(surface);
    }
}

INativeImage* WGacImageFrame::GetImage()
{
    return image;
}

Size WGacImageFrame::GetSize()
{
    return size;
}

bool WGacImageFrame::SetCache(void* key, Ptr<INativeImageFrameCache> cache)
{
    return false;
}

Ptr<INativeImageFrameCache> WGacImageFrame::GetCache(void* key)
{
    return nullptr;
}

Ptr<INativeImageFrameCache> WGacImageFrame::RemoveCache(void* key)
{
    return nullptr;
}

// WGacImage implementation
WGacImage::WGacImage(INativeImageService* service, cairo_surface_t* surface)
    : imageService(service)
    , formatType(INativeImage::Png)
{
    if (surface) {
        frames.Resize(1);
        frames[0] = Ptr(new WGacImageFrame(this, surface));
    }
}

WGacImage::~WGacImage()
{
}

INativeImageService* WGacImage::GetImageService()
{
    return imageService;
}

INativeImage::FormatType WGacImage::GetFormat()
{
    return formatType;
}

vint WGacImage::GetFrameCount()
{
    return frames.Count();
}

INativeImageFrame* WGacImage::GetFrame(vint index)
{
    if (index >= 0 && index < frames.Count()) {
        return frames[index].Obj();
    }
    return nullptr;
}

void WGacImage::SaveToStream(stream::IStream& stream, FormatType formatType)
{
    // TODO: Implement
}

// WGacImageService implementation
Ptr<INativeImage> WGacImageService::CreateImageFromFile(const WString& path)
{
    AString apath = wtoa(path);
    GError* error = nullptr;
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(apath.Buffer(), &error);

    if (!pixbuf) {
        if (error) g_error_free(error);
        return nullptr;
    }

    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    copy_pixbuf_to_surface(pixbuf, surface);
    g_object_unref(pixbuf);

    return Ptr(new WGacImage(this, surface));
}

Ptr<INativeImage> WGacImageService::CreateImageFromMemory(void* buffer, vint length)
{
    GInputStream* stream = g_memory_input_stream_new_from_data(buffer, length, nullptr);
    GError* error = nullptr;
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_stream(stream, nullptr, &error);
    g_object_unref(stream);

    if (!pixbuf) {
        if (error) g_error_free(error);
        return nullptr;
    }

    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    copy_pixbuf_to_surface(pixbuf, surface);
    g_object_unref(pixbuf);

    return Ptr(new WGacImage(this, surface));
}

Ptr<INativeImage> WGacImageService::CreateImageFromStream(stream::IStream& imageStream)
{
    stream::MemoryStream memStream;
    char buffer[4096];
    while (true) {
        vint read = imageStream.Read(buffer, sizeof(buffer));
        if (read == 0) break;
        memStream.Write(buffer, read);
    }

    return CreateImageFromMemory((void*)memStream.GetInternalBuffer(), memStream.Size());
}

}
}
}
