#include "Services/WGacImageService.h"

using namespace vl;
using namespace vl::presentation::wayland;

class InspectableImageFrame : public WGacImageFrame
{
public:
    using WGacImageFrame::WGacImageFrame;

    vint GetCacheCount() { return caches.Count(); }
    Ptr<vl::presentation::INativeImageFrameCache> GetOnlyCache() { return caches.Values()[0]; }
    void* GetOnlyCacheKey() { return caches.Keys()[0]; }
};

TEST_FILE
{
    TEST_CASE(L"Disabled images match Windows and Cocoa without changing the source")
    {
        // Two rows with padding ensure source and destination strides can differ.
        const uint32_t original[] = {
            0x00000000, 0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xDEADBEEF,
            0xFF000000, 0xFFFFFFFF, 0x80800000, 0x40102030, 0xDEADBEEF,
        };
        uint32_t pixels[10];
        for (vint i = 0; i < 10; i++) pixels[i] = original[i];
        auto* source = cairo_image_surface_create_for_data(
            reinterpret_cast<unsigned char*>(pixels), CAIRO_FORMAT_ARGB32, 4, 2, 20);
        TEST_ASSERT(cairo_surface_status(source) == CAIRO_STATUS_SUCCESS);
        InspectableImageFrame frame(nullptr, source);

        TEST_ASSERT(frame.GetSurface(true) == source);
        TEST_ASSERT(frame.GetCacheCount() == 0);

        auto* disabled = frame.GetSurface(false);
        TEST_ASSERT(frame.GetCacheCount() == 1);
        auto cache = frame.GetOnlyCache();
        TEST_ASSERT(disabled && disabled != source);
        TEST_ASSERT(cairo_surface_status(disabled) == CAIRO_STATUS_SUCCESS);
        const uint32_t expected[] = {
            0x00000000, 0xFFA9A9A9, 0xFFA9A9A9, 0xFFA9A9A9,
            0xFF7F7F7F, 0xFFFEFEFE, 0x80555555, 0x40303030,
        };
        cairo_surface_flush(disabled);
        for (vint y = 0; y < 2; y++) {
            auto* row = reinterpret_cast<const uint32_t*>(
                cairo_image_surface_get_data(disabled) + y * cairo_image_surface_get_stride(disabled));
            for (vint x = 0; x < 4; x++) TEST_ASSERT(row[x] == expected[y * 4 + x]);
        }

        TEST_ASSERT(frame.GetSurface() == source);
        TEST_ASSERT(frame.GetSurface(true) == source);
        TEST_ASSERT(frame.GetSurface(false) == disabled);
        TEST_ASSERT(frame.GetCacheCount() == 1);
        TEST_ASSERT(frame.GetOnlyCache() == cache);
        for (vint i = 0; i < 10; i++) TEST_ASSERT(pixels[i] == original[i]);

        // Transparent pixels must leave the destination untouched when painted.
        auto* target = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 4, 2);
        auto* context = cairo_create(target);
        cairo_set_source_rgb(context, 0, 0, 1);
        cairo_paint(context);
        cairo_set_source_surface(context, disabled, 0, 0);
        cairo_paint(context);
        cairo_surface_flush(target);
        auto* painted = reinterpret_cast<const uint32_t*>(cairo_image_surface_get_data(target));
        TEST_ASSERT(painted[0] == 0xFF0000FF);
        TEST_ASSERT(painted[1] == 0xFFA9A9A9);
        cairo_destroy(context);
        cairo_surface_destroy(target);

        // Detach frees the cached copy and the next disabled draw creates a new cache.
        TEST_ASSERT(frame.RemoveCache(frame.GetOnlyCacheKey()) == cache);
        TEST_ASSERT(frame.GetCacheCount() == 0);
        TEST_ASSERT(frame.GetSurface(false) != nullptr);
        TEST_ASSERT(frame.GetCacheCount() == 1);
        TEST_ASSERT(frame.GetOnlyCache() != cache);
    });

    TEST_CASE(L"Disabled images preserve every alpha value and valid premultiplied RGB")
    {
        uint32_t pixels[256];
        for (uint32_t a = 0; a < 256; a++) {
            pixels[a] = (a << 24) | (a << 16) | ((a / 2) << 8) | (a / 3);
        }
        auto* source = cairo_image_surface_create_for_data(
            reinterpret_cast<unsigned char*>(pixels), CAIRO_FORMAT_ARGB32, 256, 1, sizeof(pixels));
        WGacImageFrame frame(nullptr, source);
        auto* disabled = frame.GetSurface(false);
        TEST_ASSERT(disabled && cairo_surface_status(disabled) == CAIRO_STATUS_SUCCESS);
        cairo_surface_flush(disabled);
        auto* converted = reinterpret_cast<const uint32_t*>(cairo_image_surface_get_data(disabled));
        for (uint32_t a = 0; a < 256; a++) {
            uint32_t pixel = converted[a];
            TEST_ASSERT((pixel >> 24) == a);
            TEST_ASSERT(((pixel >> 16) & 0xFF) == (pixel & 0xFF));
            TEST_ASSERT(((pixel >> 8) & 0xFF) == (pixel & 0xFF));
            TEST_ASSERT((pixel & 0xFF) <= a);
        }
        TEST_ASSERT(converted[0] == 0);
    });
}

int main(int argc, char* argv[])
{
    auto result = unittest::UnitTest::RunAndDisposeTests(argc, argv);
    FinalizeGlobalStorage();
    return result;
}
