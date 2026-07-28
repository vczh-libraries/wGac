#include "WaylandDisplay.h"
#include "WaylandSeat.h"
#include "IWaylandWindow.h"
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <link.h>
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

    libdecor_interface libdecor_interface_ = {
        .error = WaylandDisplay::libdecor_error,
    };

    struct LibdecorPluginSearch
    {
        bool found = false;
    };

    int find_loaded_libdecor_plugin(dl_phdr_info* info, size_t, void* data)
    {
        auto* search = static_cast<LibdecorPluginSearch*>(data);
        if (!info->dlpi_name || !*info->dlpi_name)
        {
            return 0;
        }

        void* handle = dlopen(info->dlpi_name, RTLD_LAZY | RTLD_NOLOAD);
        if (!handle)
        {
            return 0;
        }

        search->found = dlsym(handle, "libdecor_plugin_description") != nullptr;
        dlclose(handle);
        return search->found ? 1 : 0;
    }

    bool has_loaded_libdecor_plugin()
    {
        LibdecorPluginSearch search;
        dl_iterate_phdr(find_loaded_libdecor_plugin, &search);
        return search.found;
    }

    int dispatch_libdecor(libdecor* context, int timeout)
    {
        int result;
        do
        {
            result = libdecor_dispatch(context, timeout);
        } while (result == -EINTR);
        return result;
    }
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

    last_error.clear();
    display = wl_display_connect(nullptr);
    if (!display) {
        ReportError("Unable to connect to the Wayland display.");
        return false;
    }

    display_fd = wl_display_get_fd(display);

    registry = wl_display_get_registry(display);
    if (!registry) {
        ReportError("Unable to obtain the Wayland registry.");
        wl_display_disconnect(display);
        display = nullptr;
        return false;
    }

    wl_registry_add_listener(registry, &registry_listener, this);

    // First roundtrip to get globals
    if (wl_display_roundtrip(display) < 0) {
        ReportError("Wayland registry discovery failed.");
        Disconnect();
        return false;
    }

    // Second roundtrip to get shm formats and other nested globals
    if (wl_display_roundtrip(display) < 0) {
        ReportError("Wayland global initialization failed.");
        Disconnect();
        return false;
    }

    if (!compositor || !shm || !xdg_wm_base_) {
        ReportError("The compositor is missing a required Wayland interface.");
        Disconnect();
        return false;
    }

    // xdg-decoration cannot guarantee that a compositor will remove a
    // server-side frame after GacUI switches to its custom frame. Force
    // libdecor to use its desktop-integrated client-side platform frame so
    // libdecor_frame_set_visibility() is authoritative for every window.
    if (setenv("LIBDECOR_FORCE_CSD", "1", 1) != 0) {
        ReportError("Unable to configure libdecor for switchable frames.");
        Disconnect();
        return false;
    }

    libdecor_context = libdecor_new(display, &libdecor_interface_);
    if (!libdecor_context) {
        ReportError("Unable to create the libdecor context.");
        Disconnect();
        return false;
    }

    // libdecor silently substitutes a no-decoration fallback if every runtime
    // plugin fails. Require a retained plugin DSO so system-frame mode can
    // never continue with a borderless raw toplevel.
    if (!has_loaded_libdecor_plugin()) {
        ReportError(
            "No usable libdecor runtime plugin was loaded. "
            "Install libdecor-0-plugin-1-gtk or another libdecor plugin.");
        Disconnect();
        return false;
    }

    // Finish libdecor's registry discovery while LIBDECOR_FORCE_CSD is set.
    if (wl_display_roundtrip(display) < 0) {
        ReportError("libdecor initialization failed.");
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

    if (libdecor_context) {
        libdecor_unref(libdecor_context);
        libdecor_context = nullptr;
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
    shm_formats.clear();
    surface_to_window.clear();
}

int WaylandDisplay::Dispatch() {
    if (libdecor_context) {
        return dispatch_libdecor(libdecor_context, -1);
    }
    return wl_display_dispatch(display);
}

int WaylandDisplay::DispatchPending() {
    if (libdecor_context) {
        return dispatch_libdecor(libdecor_context, 0);
    }
    return wl_display_dispatch_pending(display);
}

int WaylandDisplay::DispatchWithTimeout(int milliseconds) {
    if (!display) {
        return -1;
    }

    if (libdecor_context) {
        return dispatch_libdecor(libdecor_context, milliseconds);
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

    while (running) {
        if (libdecor_context) {
            if (dispatch_libdecor(libdecor_context, -1) < 0) {
                break;
            }
            continue;
        }

        pollfd pfd = {
            .fd = display_fd,
            .events = POLLIN,
            .revents = 0,
        };

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

void WaylandDisplay::ReportError(const std::string& message)
{
    last_error = message;
    fprintf(stderr, "wGac: %s\n", last_error.c_str());
}

void WaylandDisplay::libdecor_error(
    libdecor*,
    enum libdecor_error error,
    const char* message)
{
    fprintf(
        stderr,
        "wGac: fatal libdecor error (%d): %s\n",
        static_cast<int>(error),
        message ? message : "unknown error");
    fflush(stderr);

    // libdecor 0.2.x destroys its plugin immediately after this callback but
    // leaves the context pointing to it. Continuing into normal frame/context
    // cleanup would use the destroyed plugin, so take the documented fatal
    // path without returning through libdecor.
    std::_Exit(EXIT_FAILURE);
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
