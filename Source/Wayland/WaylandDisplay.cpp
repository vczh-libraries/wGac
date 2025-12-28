#include "WaylandDisplay.h"
#include "WaylandSeat.h"
#include "IWaylandWindow.h"
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <stdexcept>

namespace vl {
namespace presentation {
namespace wayland {

namespace {
    WaylandDisplay* g_wayland_display = nullptr;

    const wl_registry_listener registry_listener = {
        .global = WaylandDisplay::registry_global,
        .global_remove = WaylandDisplay::registry_global_remove,
    };

    const wl_shm_listener shm_listener = {
        .format = WaylandDisplay::shm_format,
    };

    const xdg_wm_base_listener xdg_wm_base_listener_ = {
        .ping = WaylandDisplay::xdg_wm_base_ping,
    };

    const wl_output_listener output_listener = {
        .geometry = WaylandDisplay::output_geometry,
        .mode = WaylandDisplay::output_mode,
        .done = WaylandDisplay::output_done,
        .scale = WaylandDisplay::output_scale,
    };
}

WaylandDisplay* GetWaylandDisplay() {
    return g_wayland_display;
}

void SetWaylandDisplay(WaylandDisplay* display) {
    g_wayland_display = display;
}

WaylandDisplay::WaylandDisplay() = default;

WaylandDisplay::~WaylandDisplay() {
    Disconnect();
}

bool WaylandDisplay::Connect() {
    if (connected) {
        return true;
    }

    display = wl_display_connect(nullptr);
    if (!display) {
        return false;
    }

    display_fd = wl_display_get_fd(display);

    registry = wl_display_get_registry(display);
    if (!registry) {
        wl_display_disconnect(display);
        display = nullptr;
        return false;
    }

    wl_registry_add_listener(registry, &registry_listener, this);

    // First roundtrip to get globals
    wl_display_roundtrip(display);

    // Second roundtrip to get shm formats and other nested globals
    wl_display_roundtrip(display);

    if (!compositor || !shm || !xdg_wm_base_) {
        Disconnect();
        return false;
    }

    connected = true;
    return true;
}

void WaylandDisplay::Disconnect() {
    if (!connected && !display) {
        return;
    }

    if (text_input_manager) {
        zwp_text_input_manager_v3_destroy(text_input_manager);
        text_input_manager = nullptr;
    }

    if (decoration_manager) {
        zxdg_decoration_manager_v1_destroy(decoration_manager);
        decoration_manager = nullptr;
    }

    if (xdg_wm_base_) {
        xdg_wm_base_destroy(xdg_wm_base_);
        xdg_wm_base_ = nullptr;
    }

    if (wayland_seat) {
        delete wayland_seat;
        wayland_seat = nullptr;
    }

    if (seat) {
        wl_seat_destroy(seat);
        seat = nullptr;
    }

    if (shm) {
        wl_shm_destroy(shm);
        shm = nullptr;
    }

    if (compositor) {
        wl_compositor_destroy(compositor);
        compositor = nullptr;
    }

    if (registry) {
        wl_registry_destroy(registry);
        registry = nullptr;
    }

    if (display) {
        wl_display_disconnect(display);
        display = nullptr;
    }

    display_fd = -1;
    connected = false;
    shm_formats.clear();
}

int WaylandDisplay::Dispatch() {
    return wl_display_dispatch(display);
}

int WaylandDisplay::DispatchPending() {
    return wl_display_dispatch_pending(display);
}

int WaylandDisplay::Flush() {
    return wl_display_flush(display);
}

int WaylandDisplay::Roundtrip() {
    return wl_display_roundtrip(display);
}

void WaylandDisplay::Run() {
    if (!connected) {
        return;
    }

    running = true;

    pollfd pfd = {
        .fd = display_fd,
        .events = POLLIN,
        .revents = 0,
    };

    while (running) {
        // Flush pending requests
        while (wl_display_prepare_read(display) != 0) {
            wl_display_dispatch_pending(display);
        }

        if (wl_display_flush(display) < 0) {
            wl_display_cancel_read(display);
            break;
        }

        // Wait for events
        int ret = poll(&pfd, 1, -1);
        if (ret < 0) {
            wl_display_cancel_read(display);
            break;
        }

        if (pfd.revents & POLLIN) {
            wl_display_read_events(display);
            wl_display_dispatch_pending(display);
        } else {
            wl_display_cancel_read(display);
        }

        if (pfd.revents & (POLLERR | POLLHUP)) {
            break;
        }
    }

    running = false;
}

bool WaylandDisplay::HasShmFormat(uint32_t format) const {
    for (uint32_t f : shm_formats) {
        if (f == format) {
            return true;
        }
    }
    return false;
}

void WaylandDisplay::registry_global(void* data, wl_registry* registry,
                                      uint32_t name, const char* interface, uint32_t version) {
    auto* self = static_cast<WaylandDisplay*>(data);

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        self->compositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, 4));
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0) {
        self->shm = static_cast<wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
        wl_shm_add_listener(self->shm, &shm_listener, self);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0) {
        self->seat = static_cast<wl_seat*>(
            wl_registry_bind(registry, name, &wl_seat_interface, 5));

        // Create and initialize WaylandSeat
        if (!self->wayland_seat) {
            self->wayland_seat = new WaylandSeat(self);
            self->wayland_seat->Initialize(self->seat);

            // Set up input event routing callbacks
            self->wayland_seat->SetPointerEnterCallback([](IWaylandWindow* w, int32_t x, int32_t y) {
                if (w) w->OnMouseEnter(x, y);
            });
            self->wayland_seat->SetPointerLeaveCallback([](IWaylandWindow* w) {
                if (w) w->OnMouseLeave();
            });
            self->wayland_seat->SetPointerMotionCallback([](IWaylandWindow* w, const MouseEventInfo& info) {
                if (w) w->OnMouseMove(info);
            });
            self->wayland_seat->SetPointerButtonCallback([](IWaylandWindow* w, const MouseEventInfo& info, bool pressed) {
                if (w) w->OnMouseButton(info, pressed);
            });
            self->wayland_seat->SetPointerScrollCallback([](IWaylandWindow* w, const ScrollEventInfo& info) {
                if (w) w->OnMouseScroll(info);
            });
            self->wayland_seat->SetKeyboardEnterCallback([self](IWaylandWindow* w) {
                if (w) {
                    w->OnFocusChanged(true);
                    // TODO: Re-enable text input after fixing focus loss issue
                    // Enable text input when window gains keyboard focus
                    // if (self->wayland_seat) {
                    //     self->wayland_seat->EnableTextInput(w->GetSurface(), 0, 0, 1, 20);
                    // }
                }
            });
            self->wayland_seat->SetKeyboardLeaveCallback([self](IWaylandWindow* w) {
                if (w) {
                    w->OnFocusChanged(false);
                    // TODO: Re-enable text input after fixing focus loss issue
                    // Disable text input when window loses keyboard focus
                    // if (self->wayland_seat) {
                    //     self->wayland_seat->DisableTextInput();
                    // }
                }
            });
            self->wayland_seat->SetKeyEventCallback([](IWaylandWindow* w, const KeyEventInfo& info) {
                if (w) w->OnKeyEvent(info);
            });
            self->wayland_seat->SetTextInputPreeditCallback([](IWaylandWindow* w, const PreeditInfo& info) {
                if (w) w->OnTextInputPreedit(info);
            });
            self->wayland_seat->SetTextInputCommitCallback([](IWaylandWindow* w, const std::string& text) {
                if (w) w->OnTextInputCommit(text);
            });
        }

        if (self->seat_added_callback) {
            self->seat_added_callback(self->seat, name);
        }
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        self->xdg_wm_base_ = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
        xdg_wm_base_add_listener(self->xdg_wm_base_, &xdg_wm_base_listener_, self);
    }
    else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
        self->decoration_manager = static_cast<zxdg_decoration_manager_v1*>(
            wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1));
    }
    else if (strcmp(interface, zwp_text_input_manager_v3_interface.name) == 0) {
        self->text_input_manager = static_cast<zwp_text_input_manager_v3*>(
            wl_registry_bind(registry, name, &zwp_text_input_manager_v3_interface, 1));
    }
    else if (strcmp(interface, wl_output_interface.name) == 0) {
        auto* output = static_cast<wl_output*>(
            wl_registry_bind(registry, name, &wl_output_interface, 3));
        // Use first output as primary for scale factor
        if (!self->primary_output) {
            self->primary_output = output;
        }
        wl_output_add_listener(output, &output_listener, self);
        if (self->output_added_callback) {
            self->output_added_callback(output, name);
        }
    }
}

void WaylandDisplay::registry_global_remove(void* data, wl_registry* /*registry*/, uint32_t name) {
    auto* self = static_cast<WaylandDisplay*>(data);
    if (self->output_removed_callback) {
        self->output_removed_callback(name);
    }
}

void WaylandDisplay::shm_format(void* data, wl_shm* /*shm*/, uint32_t format) {
    auto* self = static_cast<WaylandDisplay*>(data);
    self->shm_formats.push_back(format);
}

void WaylandDisplay::xdg_wm_base_ping(void* /*data*/, xdg_wm_base* xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

void WaylandDisplay::RegisterWindow(IWaylandWindow* window) {
    if (window && window->GetSurface()) {
        surface_to_window[window->GetSurface()] = window;
    }
}

void WaylandDisplay::UnregisterWindow(IWaylandWindow* window) {
    if (window && window->GetSurface()) {
        surface_to_window.erase(window->GetSurface());
    }
}

IWaylandWindow* WaylandDisplay::FindWindowBySurface(wl_surface* surface) {
    auto it = surface_to_window.find(surface);
    if (it != surface_to_window.end()) {
        return it->second;
    }
    return nullptr;
}

void WaylandDisplay::output_geometry(void* /*data*/, wl_output* /*output*/, int32_t /*x*/, int32_t /*y*/,
                                      int32_t /*physical_width*/, int32_t /*physical_height*/,
                                      int32_t /*subpixel*/, const char* /*make*/, const char* /*model*/, int32_t /*transform*/) {
    // We don't need geometry info for now
}

void WaylandDisplay::output_mode(void* /*data*/, wl_output* /*output*/, uint32_t /*flags*/,
                                  int32_t /*width*/, int32_t /*height*/, int32_t /*refresh*/) {
    // We don't need mode info for now
}

void WaylandDisplay::output_done(void* /*data*/, wl_output* /*output*/) {
    // Output info complete
}

void WaylandDisplay::output_scale(void* data, wl_output* output, int32_t factor) {
    auto* self = static_cast<WaylandDisplay*>(data);
    // Use scale from primary output
    if (output == self->primary_output) {
        self->scale_factor = factor;
    }
}

} // namespace wayland
} // namespace presentation
} // namespace vl
