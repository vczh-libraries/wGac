#ifndef WGAC_WINDOW_VIEW_H
#define WGAC_WINDOW_VIEW_H

#include "Wayland/WaylandBuffer.h"
#include <functional>

namespace vl {
namespace presentation {
namespace wayland {

class WGacWindow;

class WGacWindowView {
public:
    using DrawCallback = std::function<void(cairo_t* cr, int width, int height)>;

private:
    WGacWindow* window = nullptr;
    WaylandBufferPool* buffer_pool = nullptr;
    WaylandBuffer* current_buffer = nullptr;

    DrawCallback draw_callback;

    bool need_repaint = true;

public:
    WGacWindowView(WGacWindow* window, WaylandBufferPool* buffer_pool);
    ~WGacWindowView();

    // No copy
    WGacWindowView(const WGacWindowView&) = delete;
    WGacWindowView& operator=(const WGacWindowView&) = delete;

    // Get Cairo context for drawing
    cairo_t* GetCairoContext();

    // Get current buffer
    WaylandBuffer* GetCurrentBuffer() const { return current_buffer; }

    // Drawing
    void BeginDraw();
    void EndDraw();
    void Draw();

    // Mark as needing repaint
    void Invalidate() { need_repaint = true; }
    bool NeedsRepaint() const { return need_repaint; }

    // Set custom draw callback
    void SetDrawCallback(DrawCallback cb) { draw_callback = std::move(cb); }

    // Get dimensions
    int GetWidth() const;
    int GetHeight() const;
};

} // namespace wayland
} // namespace presentation
} // namespace vl

#endif // WGAC_WINDOW_VIEW_H
