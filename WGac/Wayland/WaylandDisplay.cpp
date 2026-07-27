#include "WaylandDisplay.h"
#include "WaylandSeat.h"
#include "IWaylandWindow.h"
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <stdexcept>

namespace vl {
namespace presentation {
namespace wayland {

namespace {
    WaylandDisplay* g_wayland_display = nullptr;

    struct DecorationProbe
    {
        bool surface_configured = false;
        bool decoration_configured = false;
        bool server_side = false;
    };

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

    void decoration_probe_surface_configure(
        void* data,
        xdg_surface* surface,
        uint32_t serial)
    {
        auto* probe = static_cast<DecorationProbe*>(data);
        probe->surface_configured = true;
        xdg_surface_ack_configure(surface, serial);
    }

    void decoration_probe_toplevel_configure(
        void*,
        xdg_toplevel*,
        int32_t,
        int32_t,
        wl_array*)
    {
    }

    void decoration_probe_toplevel_close(void*, xdg_toplevel*)
    {
    }

    void decoration_probe_toplevel_configure_bounds(
        void*,
        xdg_toplevel*,
        int32_t,
        int32_t)
    {
    }

    void decoration_probe_toplevel_wm_capabilities(
        void*,
        xdg_toplevel*,
        wl_array*)
    {
    }

    void decoration_probe_configure(
        void* data,
        zxdg_toplevel_decoration_v1*,
        uint32_t mode)
    {
        auto* probe = static_cast<DecorationProbe*>(data);
        probe->decoration_configured = true;
        probe->server_side =
            mode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE;
    }

    const xdg_surface_listener decoration_probe_surface_listener = {
        .configure = decoration_probe_surface_configure,
    };

    const xdg_toplevel_listener decoration_probe_toplevel_listener = {
        .configure = decoration_probe_toplevel_configure,
        .close = decoration_probe_toplevel_close,
        .configure_bounds = decoration_probe_toplevel_configure_bounds,
        .wm_capabilities = decoration_probe_toplevel_wm_capabilities,
    };

    const zxdg_toplevel_decoration_v1_listener decoration_probe_listener = {
        .configure = decoration_probe_configure,
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

    if (!ProbeDecorationMode()) {
        Disconnect();
        return false;
    }

    connected = true;
    return true;
}

bool WaylandDisplay::ProbeDecorationMode()
{
    prefer_custom_frame_window = true;
    if (!decoration_manager)
    {
        return true;
    }

    wl_surface* probeSurface = wl_compositor_create_surface(compositor);
    xdg_surface* probeXdgSurface = probeSurface
        ? xdg_wm_base_get_xdg_surface(xdg_wm_base_, probeSurface)
        : nullptr;
    xdg_toplevel* probeToplevel = probeXdgSurface
        ? xdg_surface_get_toplevel(probeXdgSurface)
        : nullptr;
    zxdg_toplevel_decoration_v1* probeDecoration = probeToplevel
        ? zxdg_decoration_manager_v1_get_toplevel_decoration(
            decoration_manager,
            probeToplevel)
        : nullptr;

    DecorationProbe probe;
    if (probeXdgSurface)
    {
        xdg_surface_add_listener(
            probeXdgSurface,
            &decoration_probe_surface_listener,
            &probe);
    }
    if (probeToplevel)
    {
        xdg_toplevel_add_listener(
            probeToplevel,
            &decoration_probe_toplevel_listener,
            &probe);
        xdg_toplevel_set_title(probeToplevel, "GacUI Window");
        xdg_toplevel_set_app_id(probeToplevel, "gacui");
    }
    if (probeDecoration)
    {
        zxdg_toplevel_decoration_v1_add_listener(
            probeDecoration,
            &decoration_probe_listener,
            &probe);
        zxdg_toplevel_decoration_v1_set_mode(
            probeDecoration,
            ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
        wl_surface_commit(probeSurface);

        for (int attempt = 0;
            attempt < 2 &&
                (!probe.decoration_configured || !probe.surface_configured);
            attempt++)
        {
            if (wl_display_roundtrip(display) < 0)
            {
                break;
            }
        }
    }

    prefer_custom_frame_window =
        !probe.surface_configured ||
        !probe.decoration_configured ||
        !probe.server_side;

    if (probeDecoration)
    {
        zxdg_toplevel_decoration_v1_destroy(probeDecoration);
    }
    if (probeToplevel)
    {
        xdg_toplevel_destroy(probeToplevel);
    }
    if (probeXdgSurface)
    {
        xdg_surface_destroy(probeXdgSurface);
    }
    if (probeSurface)
    {
        wl_surface_destroy(probeSurface);
    }

    return wl_display_get_error(display) == 0;
}

void WaylandDisplay::Disconnect() {
    if (!connected && !display) {
        return;
    }

    if (data_device) {
        wl_data_device_destroy(data_device);
        data_device = nullptr;
    }

    if (data_device_manager) {
        wl_data_device_manager_destroy(data_device_manager);
        data_device_manager = nullptr;
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
    prefer_custom_frame_window = true;
    shm_formats.clear();
}

int WaylandDisplay::Dispatch() {
    return wl_display_dispatch(display);
}

int WaylandDisplay::DispatchPending() {
    return wl_display_dispatch_pending(display);
}

int WaylandDisplay::DispatchWithTimeout(int milliseconds) {
    if (!display) {
        return -1;
    }

    while (wl_display_prepare_read(display) != 0) {
        if (wl_display_dispatch_pending(display) < 0) {
            return -1;
        }
    }

    int flushResult = wl_display_flush(display);
    if (flushResult < 0 && errno != EAGAIN) {
        wl_display_cancel_read(display);
        return -1;
    }

    pollfd pfd = {
        .fd = display_fd,
        .events = static_cast<short>(POLLIN | (flushResult < 0 ? POLLOUT : 0)),
        .revents = 0,
    };

    int pollResult;
    do {
        pollResult = poll(&pfd, 1, milliseconds);
    } while (pollResult < 0 && errno == EINTR);

    if (pollResult <= 0) {
        wl_display_cancel_read(display);
        return pollResult;
    }

    if (pfd.revents & POLLIN) {
        if (wl_display_read_events(display) < 0) {
            return -1;
        }
    } else {
        wl_display_cancel_read(display);
    }

    if ((pfd.revents & POLLOUT) && wl_display_flush(display) < 0 && errno != EAGAIN) {
        return -1;
    }
    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        return -1;
    }

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
                    // Enable text input when window gains keyboard focus
                    if (self->wayland_seat) {
                        self->wayland_seat->EnableTextInput(w->GetSurface(), 0, 0, 1, 20);
                    }
                }
            });
            self->wayland_seat->SetKeyboardLeaveCallback([self](IWaylandWindow* w) {
                if (w) {
                    w->OnFocusChanged(false);
                    // Disable text input when window loses keyboard focus
                    if (self->wayland_seat) {
                        self->wayland_seat->DisableTextInput();
                    }
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

        // Create data_device if data_device_manager is already available
        if (self->data_device_manager && !self->data_device) {
            self->data_device = wl_data_device_manager_get_data_device(
                self->data_device_manager, self->seat);
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
    else if (strcmp(interface, wl_data_device_manager_interface.name) == 0) {
        self->data_device_manager = static_cast<wl_data_device_manager*>(
            wl_registry_bind(registry, name, &wl_data_device_manager_interface, 3));
        // Create data_device if seat is already available
        if (self->seat && !self->data_device) {
            self->data_device = wl_data_device_manager_get_data_device(
                self->data_device_manager, self->seat);
        }
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
