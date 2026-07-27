#include "WGacView.h"
#include "WGacWindow.h"

namespace vl {
namespace presentation {
namespace wayland {

WGacView::WGacView(WGacWindow* window, WaylandBufferPool* buffer_pool)
    : window(window)
    , buffer_pool(buffer_pool) {}

WGacView::~WGacView() {
    current_buffer = nullptr;
}

cairo_t* WGacView::GetCairoContext() {
    if (!current_buffer) {
        current_buffer = buffer_pool->GetNextBuffer();
    }

    if (current_buffer) {
        return current_buffer->GetCairoContext();
    }

    return nullptr;
}

void WGacView::BeginDraw() {
    current_buffer = buffer_pool->GetNextBuffer();
    if (current_buffer) {
        current_buffer->BeginDraw();
    }
}

void WGacView::EndDraw() {
    if (current_buffer) {
        current_buffer->EndDraw();
    }
    need_repaint = false;
}

void WGacView::Draw() {
    BeginDraw();

    cairo_t* cr = GetCairoContext();
    if (!cr) {
        return;
    }

    int width = GetWidth();
    int height = GetHeight();

    // Clear background (default: white)
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    // Call custom draw callback if set
    if (draw_callback) {
        draw_callback(cr, width, height);
    }

    EndDraw();
}

int WGacView::GetWidth() const {
    return buffer_pool ? static_cast<int>(buffer_pool->GetWidth()) : 0;
}

int WGacView::GetHeight() const {
    return buffer_pool ? static_cast<int>(buffer_pool->GetHeight()) : 0;
}

} // namespace wayland
} // namespace presentation
} // namespace vl
