#include "WGacWindow.h"
#include "WGacWindowView.h"
#include <cstring>

namespace vl {
namespace presentation {
namespace wayland {

namespace {
    const xdg_surface_listener xdg_surface_listener_ = {
        .configure = WGacWindow::xdg_surface_configure,
    };

    const xdg_toplevel_listener xdg_toplevel_listener_ = {
        .configure = WGacWindow::xdg_toplevel_configure,
        .close = WGacWindow::xdg_toplevel_close,
        .configure_bounds = WGacWindow::xdg_toplevel_configure_bounds,
        .wm_capabilities = WGacWindow::xdg_toplevel_wm_capabilities,
    };

    const wl_callback_listener frame_listener = {
        .done = WGacWindow::frame_done,
    };
}

WGacWindow::WGacWindow(WaylandDisplay* display)
    : display(display) {}

WGacWindow::~WGacWindow() {
    Destroy();
}

bool WGacWindow::Create(const WindowConfig& cfg) {
    if (surface) {
        return false;  // Already created
    }

    config = cfg;
    current_width = config.width;
    current_height = config.height;

    // Create wl_surface
    surface = wl_compositor_create_surface(display->GetCompositor());
    if (!surface) {
        return false;
    }

    // Create xdg_surface
    xdg_surface_ = xdg_wm_base_get_xdg_surface(display->GetXdgWmBase(), surface);
    if (!xdg_surface_) {
        wl_surface_destroy(surface);
        surface = nullptr;
        return false;
    }
    xdg_surface_add_listener(xdg_surface_, &xdg_surface_listener_, this);

    // Create xdg_toplevel
    toplevel = xdg_surface_get_toplevel(xdg_surface_);
    if (!toplevel) {
        xdg_surface_destroy(xdg_surface_);
        wl_surface_destroy(surface);
        xdg_surface_ = nullptr;
        surface = nullptr;
        return false;
    }
    xdg_toplevel_add_listener(toplevel, &xdg_toplevel_listener_, this);

    // Set initial properties
    xdg_toplevel_set_title(toplevel, config.title.c_str());
    xdg_toplevel_set_app_id(toplevel, "wgac");

    if (config.min_width > 0 && config.min_height > 0) {
        xdg_toplevel_set_min_size(toplevel, config.min_width, config.min_height);
    }

    // Request server-side decorations if available
    if (config.decorated && display->GetDecorationManager()) {
        decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
            display->GetDecorationManager(), toplevel);
        if (decoration) {
            zxdg_toplevel_decoration_v1_set_mode(
                decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
        }
    }

    // Create buffer pool
    buffer_pool = new WaylandBufferPool(display->GetShm());
    if (!buffer_pool->Resize(current_width, current_height)) {
        Destroy();
        return false;
    }

    // Create view
    view = new WGacWindowView(this, buffer_pool);

    // Register with display for input event routing
    display->RegisterWindow(this);

    // Commit the initial surface state
    wl_surface_commit(surface);

    return true;
}

void WGacWindow::Destroy() {
    // Unregister from display first
    if (display && surface) {
        display->UnregisterWindow(this);
    }

    if (frame_callback) {
        wl_callback_destroy(frame_callback);
        frame_callback = nullptr;
    }

    delete view;
    view = nullptr;

    delete buffer_pool;
    buffer_pool = nullptr;

    if (decoration) {
        zxdg_toplevel_decoration_v1_destroy(decoration);
        decoration = nullptr;
    }

    if (toplevel) {
        xdg_toplevel_destroy(toplevel);
        toplevel = nullptr;
    }

    if (xdg_surface_) {
        xdg_surface_destroy(xdg_surface_);
        xdg_surface_ = nullptr;
    }

    if (surface) {
        wl_surface_destroy(surface);
        surface = nullptr;
    }

    configured = false;
    visible = false;
    closed = false;
    has_first_frame = false;
}

void WGacWindow::Show() {
    if (!surface || visible) {
        return;
    }

    visible = true;

    // Initial draw if configured
    if (configured) {
        Invalidate();
    }
}

void WGacWindow::Hide() {
    if (!surface || !visible) {
        return;
    }

    visible = false;

    // Hide by attaching a null buffer
    wl_surface_attach(surface, nullptr, 0, 0);
    wl_surface_commit(surface);
}

void WGacWindow::SetSize(int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    // For xdg_toplevel, we can't set size directly
    // We can only suggest a size, but compositor decides
    // This is mainly useful before the first configure
    config.width = width;
    config.height = height;
}

void WGacWindow::SetMinSize(int32_t width, int32_t height) {
    config.min_width = width;
    config.min_height = height;
    if (toplevel) {
        xdg_toplevel_set_min_size(toplevel, width, height);
    }
}

void WGacWindow::SetMaxSize(int32_t width, int32_t height) {
    if (toplevel) {
        xdg_toplevel_set_max_size(toplevel, width, height);
    }
}

void WGacWindow::SetTitle(const std::string& title) {
    config.title = title;
    if (toplevel) {
        xdg_toplevel_set_title(toplevel, title.c_str());
    }
}

void WGacWindow::Maximize() {
    if (toplevel) {
        xdg_toplevel_set_maximized(toplevel);
    }
}

void WGacWindow::Minimize() {
    if (toplevel) {
        xdg_toplevel_set_minimized(toplevel);
    }
}

void WGacWindow::Restore() {
    if (toplevel) {
        xdg_toplevel_unset_maximized(toplevel);
    }
}

void WGacWindow::Invalidate() {
    if (!visible || !configured) {
        return;
    }
    RequestFrame();
}

void WGacWindow::RequestFrame() {
    if (pending_frame || !surface) {
        return;
    }

    pending_frame = true;

    // For the first frame, we need to draw and attach buffer immediately
    // because Wayland compositor won't send frame events for surfaces without content
    if (!has_first_frame) {
        has_first_frame = true;
        // Draw content immediately
        if (view) {
            view->Draw();
        }
        // Commit with buffer attached
        Commit();
        // Now request next frame callback
        frame_callback = wl_surface_frame(surface);
        wl_callback_add_listener(frame_callback, &frame_listener, this);
        wl_surface_commit(surface);
    } else {
        // Subsequent frames: request callback first, then draw in OnFrame
        frame_callback = wl_surface_frame(surface);
        wl_callback_add_listener(frame_callback, &frame_listener, this);
        wl_surface_commit(surface);
    }
}

void WGacWindow::OnFrame() {
    pending_frame = false;
    frame_callback = nullptr;

    if (!visible || !configured) {
        return;
    }

    // Call frame handler for rendering
    if (frame_handler) {
        frame_handler();
    }

    // Default: draw via view
    if (view) {
        view->Draw();
    }

    Commit();
}

void WGacWindow::Commit() {
    if (!surface || !view) {
        return;
    }

    WaylandBuffer* buffer = view->GetCurrentBuffer();
    if (buffer) {
        buffer->EndDraw();
        buffer->Attach(surface, 0, 0);
        buffer->DamageAll(surface);
        wl_surface_commit(surface);
    }
}

// Static callbacks

void WGacWindow::xdg_surface_configure(void* data, xdg_surface* xdg_surface, uint32_t serial) {
    auto* self = static_cast<WGacWindow*>(data);

    xdg_surface_ack_configure(xdg_surface, serial);

    bool first_configure = !self->configured;
    self->configured = true;

    // Resize buffers if needed
    if (self->current_width > 0 && self->current_height > 0) {
        if (self->buffer_pool->GetWidth() != static_cast<uint32_t>(self->current_width) ||
            self->buffer_pool->GetHeight() != static_cast<uint32_t>(self->current_height)) {

            self->buffer_pool->Resize(self->current_width, self->current_height);

            if (self->resize_callback) {
                self->resize_callback(self->current_width, self->current_height);
            }
        }
    }

    // Initial draw on first configure
    if (first_configure && self->visible) {
        self->Invalidate();
    }
}

void WGacWindow::xdg_toplevel_configure(void* data, xdg_toplevel* /*toplevel*/,
                                         int32_t width, int32_t height, wl_array* states) {
    auto* self = static_cast<WGacWindow*>(data);

    // Parse states
    self->size_state = WindowSizeState::Restored;
    auto* state_ptr = static_cast<uint32_t*>(states->data);
    size_t num_states = states->size / sizeof(uint32_t);
    for (size_t i = 0; i < num_states; ++i) {
        switch (state_ptr[i]) {
            case XDG_TOPLEVEL_STATE_MAXIMIZED:
                self->size_state = WindowSizeState::Maximized;
                break;
            case XDG_TOPLEVEL_STATE_FULLSCREEN:
                // Treat as maximized for now
                self->size_state = WindowSizeState::Maximized;
                break;
            default:
                break;
        }
    }

    // Size of 0 means compositor doesn't care, use our preferred size
    if (width > 0 && height > 0) {
        self->current_width = width;
        self->current_height = height;
    } else if (self->current_width == 0 || self->current_height == 0) {
        self->current_width = self->config.width;
        self->current_height = self->config.height;
    }
}

void WGacWindow::xdg_toplevel_close(void* data, xdg_toplevel* /*toplevel*/) {
    auto* self = static_cast<WGacWindow*>(data);
    self->closed = true;

    if (self->close_callback) {
        self->close_callback();
    }
}

void WGacWindow::xdg_toplevel_configure_bounds(void* /*data*/, xdg_toplevel* /*toplevel*/,
                                                int32_t /*width*/, int32_t /*height*/) {
    // Optional: could use this to limit window size
}

void WGacWindow::xdg_toplevel_wm_capabilities(void* /*data*/, xdg_toplevel* /*toplevel*/,
                                               wl_array* /*capabilities*/) {
    // Optional: could check what WM capabilities are available
}

void WGacWindow::frame_done(void* data, wl_callback* callback, uint32_t /*time*/) {
    auto* self = static_cast<WGacWindow*>(data);
    wl_callback_destroy(callback);
    self->OnFrame();
}

// Input event handlers
void WGacWindow::OnMouseMove(const MouseEventInfo& info) {
    if (mouse_move_callback) {
        mouse_move_callback(info);
    }
}

void WGacWindow::OnMouseButton(const MouseEventInfo& info, bool pressed) {
    if (mouse_button_callback) {
        mouse_button_callback(info, pressed);
    }
}

void WGacWindow::OnMouseScroll(const ScrollEventInfo& info) {
    if (mouse_scroll_callback) {
        mouse_scroll_callback(info);
    }
}

void WGacWindow::OnMouseEnter(int32_t x, int32_t y) {
    if (mouse_enter_callback) {
        mouse_enter_callback(x, y);
    }
}

void WGacWindow::OnMouseLeave() {
    if (mouse_leave_callback) {
        mouse_leave_callback();
    }
}

void WGacWindow::OnKeyEvent(const KeyEventInfo& info) {
    if (keyboard_callback) {
        keyboard_callback(info);
    }
}

void WGacWindow::OnFocusChanged(bool focused) {
    if (focus_callback) {
        focus_callback(focused);
    }
}

} // namespace wayland
} // namespace presentation
} // namespace vl
