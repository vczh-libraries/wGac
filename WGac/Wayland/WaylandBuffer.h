#ifndef WGAC_WAYLAND_BUFFER_H
#define WGAC_WAYLAND_BUFFER_H

#include <wayland-client.h>
#include <cairo/cairo.h>
#include <cstdint>

namespace vl {
namespace presentation {
namespace wayland {

class WaylandBuffer {
private:
    wl_shm_pool* pool = nullptr;
    wl_buffer* buffer = nullptr;
    void* data = nullptr;
    int fd = -1;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride = 0;
    size_t size = 0;

    cairo_surface_t* cairo_surface = nullptr;
    cairo_t* cairo_context = nullptr;

    bool busy = false;  // True when compositor is using this buffer

    WaylandBuffer() = default;

public:
    // Wayland callback (must be public for C linkage)
    static void buffer_release(void* data, wl_buffer* buffer);

public:
    ~WaylandBuffer();

    // No copy
    WaylandBuffer(const WaylandBuffer&) = delete;
    WaylandBuffer& operator=(const WaylandBuffer&) = delete;

    // Move is allowed
    WaylandBuffer(WaylandBuffer&& other) noexcept;
    WaylandBuffer& operator=(WaylandBuffer&& other) noexcept;

    static WaylandBuffer* Create(wl_shm* shm, uint32_t width, uint32_t height);
    void Destroy();

    // Getters
    wl_buffer* GetBuffer() const { return buffer; }
    void* GetData() const { return data; }
    uint32_t GetWidth() const { return width; }
    uint32_t GetHeight() const { return height; }
    uint32_t GetStride() const { return stride; }
    size_t GetSize() const { return size; }

    // Cairo integration
    cairo_surface_t* GetCairoSurface() const { return cairo_surface; }
    cairo_t* GetCairoContext() const { return cairo_context; }

    // Buffer state
    bool IsBusy() const { return busy; }
    void SetBusy(bool b) { busy = b; }

    // Attach to surface
    void Attach(wl_surface* surface, int32_t x = 0, int32_t y = 0);

    // Mark damage region
    void Damage(wl_surface* surface, int32_t x, int32_t y, int32_t w, int32_t h);
    void DamageAll(wl_surface* surface);

    // Begin/End drawing (flushes cairo)
    void BeginDraw();
    void EndDraw();
};

class WaylandBufferPool {
private:
    wl_shm* shm;
    WaylandBuffer* buffers[2] = {nullptr, nullptr};
    int current = 0;
    uint32_t width = 0;
    uint32_t height = 0;

public:
    explicit WaylandBufferPool(wl_shm* shm);
    ~WaylandBufferPool();

    bool Resize(uint32_t new_width, uint32_t new_height);
    WaylandBuffer* GetNextBuffer();

    uint32_t GetWidth() const { return width; }
    uint32_t GetHeight() const { return height; }
};

} // namespace wayland
} // namespace presentation
} // namespace vl

#endif // WGAC_WAYLAND_BUFFER_H
