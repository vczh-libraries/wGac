#include "WGacImageService.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "../ThirdParty/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include "../ThirdParty/stb_image_write.h"

#include <cstring>
#include <fstream>
#include <vector>

namespace vl {
namespace presentation {
namespace wayland {

// Helper function to copy RGBA pixels to cairo surface (ARGB32 with premultiplied alpha)
static void copy_rgba_to_surface(const unsigned char* pixels, int width, int height, cairo_surface_t* surface)
{
    unsigned char* dest = cairo_image_surface_get_data(surface);
    int dest_stride = cairo_image_surface_get_stride(surface);

    cairo_surface_flush(surface);

    for (int y = 0; y < height; y++) {
        const unsigned char* src_row = pixels + y * width * 4;
        uint32_t* dest_row = (uint32_t*)(dest + y * dest_stride);

        for (int x = 0; x < width; x++) {
            unsigned char r = src_row[x * 4 + 0];
            unsigned char g = src_row[x * 4 + 1];
            unsigned char b = src_row[x * 4 + 2];
            unsigned char a = src_row[x * 4 + 3];

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

// Helper to read file into memory
static std::vector<unsigned char> read_file(const char* path)
{
    std::vector<unsigned char> data;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return data;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    data.resize(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

// Callback for stb_image_write to write to vector
static void stbi_write_callback(void* context, void* data, int size)
{
    std::vector<unsigned char>* vec = static_cast<std::vector<unsigned char>*>(context);
    unsigned char* bytes = static_cast<unsigned char*>(data);
    vec->insert(vec->end(), bytes, bytes + size);
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
    if (caches.Keys().Contains(key)) {
        return false;
    }
    caches.Add(key, cache);
    return true;
}

Ptr<INativeImageFrameCache> WGacImageFrame::GetCache(void* key)
{
    vint index = caches.Keys().IndexOf(key);
    if (index != -1) {
        return caches.Values()[index];
    }
    return nullptr;
}

Ptr<INativeImageFrameCache> WGacImageFrame::RemoveCache(void* key)
{
    vint index = caches.Keys().IndexOf(key);
    if (index != -1) {
        Ptr<INativeImageFrameCache> cache = caches.Values()[index];
        caches.Remove(key);
        return cache;
    }
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

void WGacImage::SaveToStream(stream::IStream& imageStream, FormatType format)
{
    if (frames.Count() == 0) return;

    WGacImageFrame* frame = frames[0].Obj();
    cairo_surface_t* surface = frame->GetSurface();
    if (!surface) return;

    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);
    unsigned char* data = cairo_image_surface_get_data(surface);

    // Convert from Cairo ARGB32 (premultiplied) to RGBA
    std::vector<unsigned char> rgba(width * height * 4);
    for (int y = 0; y < height; y++) {
        uint32_t* src_row = (uint32_t*)(data + y * stride);
        unsigned char* dest_row = rgba.data() + y * width * 4;

        for (int x = 0; x < width; x++) {
            uint32_t pixel = src_row[x];
            unsigned char a = (pixel >> 24) & 0xFF;
            unsigned char r = (pixel >> 16) & 0xFF;
            unsigned char g = (pixel >> 8) & 0xFF;
            unsigned char b = pixel & 0xFF;

            // Unpremultiply alpha
            if (a > 0 && a < 255) {
                r = r * 255 / a;
                g = g * 255 / a;
                b = b * 255 / a;
            }

            dest_row[x * 4 + 0] = r;
            dest_row[x * 4 + 1] = g;
            dest_row[x * 4 + 2] = b;
            dest_row[x * 4 + 3] = a;
        }
    }

    std::vector<unsigned char> output;

    switch (format) {
        case INativeImage::Png:
            stbi_write_png_to_func(stbi_write_callback, &output, width, height, 4, rgba.data(), width * 4);
            break;
        case INativeImage::Jpeg:
            stbi_write_jpg_to_func(stbi_write_callback, &output, width, height, 4, rgba.data(), 90);
            break;
        case INativeImage::Bmp:
            stbi_write_bmp_to_func(stbi_write_callback, &output, width, height, 4, rgba.data());
            break;
        default:
            // Default to PNG
            stbi_write_png_to_func(stbi_write_callback, &output, width, height, 4, rgba.data(), width * 4);
            break;
    }

    if (!output.empty()) {
        imageStream.Write(output.data(), output.size());
    }
}

// WGacImageService implementation
Ptr<INativeImage> WGacImageService::CreateImageFromFile(const WString& path)
{
    AString apath = wtoa(path);
    std::vector<unsigned char> fileData = read_file(apath.Buffer());

    if (fileData.empty()) {
        return nullptr;
    }

    return CreateImageFromMemory(fileData.data(), fileData.size());
}

Ptr<INativeImage> WGacImageService::CreateImageFromMemory(void* buffer, vint length)
{
    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(
        static_cast<const unsigned char*>(buffer),
        static_cast<int>(length),
        &width, &height, &channels, 4  // Force RGBA
    );

    if (!pixels) {
        return nullptr;
    }

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        stbi_image_free(pixels);
        return nullptr;
    }

    copy_rgba_to_surface(pixels, width, height, surface);
    stbi_image_free(pixels);

    return Ptr(new WGacImage(this, surface));
}

Ptr<INativeImage> WGacImageService::CreateImageFromStream(stream::IStream& imageStream)
{
    std::vector<char> buffer;
    char chunk[4096];
    while (true) {
        vint bytesRead = imageStream.Read(chunk, sizeof(chunk));
        if (bytesRead == 0) break;
        buffer.insert(buffer.end(), chunk, chunk + bytesRead);
    }

    if (buffer.empty()) {
        return nullptr;
    }

    return CreateImageFromMemory(buffer.data(), buffer.size());
}

}
}
}
