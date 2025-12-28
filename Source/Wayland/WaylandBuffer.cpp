#include "WaylandBuffer.h"
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctime>
#include <cerrno>

namespace vl {
namespace presentation {
namespace wayland {

namespace {
    const wl_buffer_listener buffer_listener = {
        .release = WaylandBuffer::buffer_release,
    };

    void randname(char* buf) {
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        long r = ts.tv_nsec;
        for (int i = 0; i < 6; ++i) {
            buf[i] = 'A' + (r & 15) + (r & 16) * 2;
            r >>= 5;
        }
    }

    int create_shm_file() {
        int retries = 100;
        do {
            char name[] = "/wl_shm-XXXXXX";
            randname(name + sizeof(name) - 7);
            --retries;
            int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
            if (fd >= 0) {
                shm_unlink(name);
                return fd;
            }
        } while (retries > 0 && errno == EEXIST);
        return -1;
    }

    int allocate_shm_file(size_t size) {
        int fd = create_shm_file();
        if (fd < 0) {
            return -1;
        }
        int ret;
        do {
            ret = ftruncate(fd, size);
        } while (ret < 0 && errno == EINTR);
        if (ret < 0) {
            close(fd);
            return -1;
        }
        return fd;
    }
}

void WaylandBuffer::buffer_release(void* data, wl_buffer* /*buffer*/) {
    auto* self = static_cast<WaylandBuffer*>(data);
    self->busy = false;
}

WaylandBuffer::~WaylandBuffer() {
    Destroy();
}

WaylandBuffer::WaylandBuffer(WaylandBuffer&& other) noexcept
    : pool(other.pool)
    , buffer(other.buffer)
    , data(other.data)
    , fd(other.fd)
    , width(other.width)
    , height(other.height)
    , stride(other.stride)
    , size(other.size)
    , cairo_surface(other.cairo_surface)
    , cairo_context(other.cairo_context)
    , busy(other.busy)
{
    other.pool = nullptr;
    other.buffer = nullptr;
    other.data = nullptr;
    other.fd = -1;
    other.cairo_surface = nullptr;
    other.cairo_context = nullptr;
}

WaylandBuffer& WaylandBuffer::operator=(WaylandBuffer&& other) noexcept {
    if (this != &other) {
        Destroy();

        pool = other.pool;
        buffer = other.buffer;
        data = other.data;
        fd = other.fd;
        width = other.width;
        height = other.height;
        stride = other.stride;
        size = other.size;
        cairo_surface = other.cairo_surface;
        cairo_context = other.cairo_context;
        busy = other.busy;

        other.pool = nullptr;
        other.buffer = nullptr;
        other.data = nullptr;
        other.fd = -1;
        other.cairo_surface = nullptr;
        other.cairo_context = nullptr;
    }
    return *this;
}

WaylandBuffer* WaylandBuffer::Create(wl_shm* shm, uint32_t width, uint32_t height) {
    if (!shm || width == 0 || height == 0) {
        return nullptr;
    }

    auto* buf = new WaylandBuffer();
    buf->width = width;
    buf->height = height;
    buf->stride = width * 4;  // ARGB32
    buf->size = buf->stride * height;

    // Allocate shared memory
    buf->fd = allocate_shm_file(buf->size);
    if (buf->fd < 0) {
        delete buf;
        return nullptr;
    }

    // Map memory
    buf->data = mmap(nullptr, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED, buf->fd, 0);
    if (buf->data == MAP_FAILED) {
        close(buf->fd);
        delete buf;
        return nullptr;
    }

    // Create shm pool
    buf->pool = wl_shm_create_pool(shm, buf->fd, buf->size);
    if (!buf->pool) {
        munmap(buf->data, buf->size);
        close(buf->fd);
        delete buf;
        return nullptr;
    }

    // Create buffer from pool
    buf->buffer = wl_shm_pool_create_buffer(
        buf->pool, 0,
        width, height, buf->stride,
        WL_SHM_FORMAT_ARGB8888
    );
    if (!buf->buffer) {
        wl_shm_pool_destroy(buf->pool);
        munmap(buf->data, buf->size);
        close(buf->fd);
        delete buf;
        return nullptr;
    }

    wl_buffer_add_listener(buf->buffer, &buffer_listener, buf);

    // Create Cairo surface backed by this buffer
    buf->cairo_surface = cairo_image_surface_create_for_data(
        static_cast<unsigned char*>(buf->data),
        CAIRO_FORMAT_ARGB32,
        width, height, buf->stride
    );
    if (cairo_surface_status(buf->cairo_surface) != CAIRO_STATUS_SUCCESS) {
        wl_buffer_destroy(buf->buffer);
        wl_shm_pool_destroy(buf->pool);
        munmap(buf->data, buf->size);
        close(buf->fd);
        delete buf;
        return nullptr;
    }

    // Create Cairo context
    buf->cairo_context = cairo_create(buf->cairo_surface);
    if (cairo_status(buf->cairo_context) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(buf->cairo_surface);
        wl_buffer_destroy(buf->buffer);
        wl_shm_pool_destroy(buf->pool);
        munmap(buf->data, buf->size);
        close(buf->fd);
        delete buf;
        return nullptr;
    }

    return buf;
}

void WaylandBuffer::Destroy() {
    if (cairo_context) {
        cairo_destroy(cairo_context);
        cairo_context = nullptr;
    }

    if (cairo_surface) {
        cairo_surface_destroy(cairo_surface);
        cairo_surface = nullptr;
    }

    if (buffer) {
        wl_buffer_destroy(buffer);
        buffer = nullptr;
    }

    if (pool) {
        wl_shm_pool_destroy(pool);
        pool = nullptr;
    }

    if (data && data != MAP_FAILED) {
        munmap(data, size);
        data = nullptr;
    }

    if (fd >= 0) {
        close(fd);
        fd = -1;
    }

    width = 0;
    height = 0;
    stride = 0;
    size = 0;
    busy = false;
}

void WaylandBuffer::Attach(wl_surface* surface, int32_t x, int32_t y) {
    busy = true;
    wl_surface_attach(surface, buffer, x, y);
}

void WaylandBuffer::Damage(wl_surface* surface, int32_t x, int32_t y, int32_t w, int32_t h) {
    wl_surface_damage_buffer(surface, x, y, w, h);
}

void WaylandBuffer::DamageAll(wl_surface* surface) {
    Damage(surface, 0, 0, width, height);
}

void WaylandBuffer::BeginDraw() {
    // Nothing special needed, just ensure cairo is ready
}

void WaylandBuffer::EndDraw() {
    // Flush cairo drawing to the surface
    cairo_surface_flush(cairo_surface);
}

// WaylandBufferPool implementation

WaylandBufferPool::WaylandBufferPool(wl_shm* shm)
    : shm(shm) {}

WaylandBufferPool::~WaylandBufferPool() {
    delete buffers[0];
    delete buffers[1];
}

bool WaylandBufferPool::Resize(uint32_t new_width, uint32_t new_height) {
    if (new_width == width && new_height == height) {
        return true;
    }

    // Destroy old buffers
    delete buffers[0];
    delete buffers[1];
    buffers[0] = nullptr;
    buffers[1] = nullptr;

    width = new_width;
    height = new_height;

    // Create new buffers
    buffers[0] = WaylandBuffer::Create(shm, width, height);
    buffers[1] = WaylandBuffer::Create(shm, width, height);

    if (!buffers[0] || !buffers[1]) {
        delete buffers[0];
        delete buffers[1];
        buffers[0] = nullptr;
        buffers[1] = nullptr;
        width = 0;
        height = 0;
        return false;
    }

    current = 0;
    return true;
}

WaylandBuffer* WaylandBufferPool::GetNextBuffer() {
    // Try current buffer
    if (buffers[current] && !buffers[current]->IsBusy()) {
        return buffers[current];
    }

    // Try other buffer
    int other = 1 - current;
    if (buffers[other] && !buffers[other]->IsBusy()) {
        current = other;
        return buffers[other];
    }

    // Both buffers busy - this shouldn't happen with proper frame callbacks
    // Return current anyway (will cause tearing)
    return buffers[current];
}

} // namespace wayland
} // namespace presentation
} // namespace vl
