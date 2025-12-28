#ifndef WGAC_WAYLAND_DISPLAY_H
#define WGAC_WAYLAND_DISPLAY_H

#include <wayland-client.h>
#include "../Protocol/xdg-shell-client-protocol.h"
#include "../Protocol/xdg-decoration-unstable-v1-client-protocol.h"
#include "../Protocol/text-input-unstable-v3-client-protocol.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>

namespace vl {
namespace presentation {
namespace wayland {

class WGacWindow;
class WGacNativeWindow;
class WaylandSeat;
class IWaylandWindow;

class WaylandDisplay {
public:
    using OutputAddedCallback = std::function<void(wl_output*, uint32_t name)>;
    using OutputRemovedCallback = std::function<void(uint32_t name)>;
    using SeatAddedCallback = std::function<void(wl_seat*, uint32_t name)>;

private:
    wl_display* display = nullptr;
    wl_registry* registry = nullptr;
    wl_compositor* compositor = nullptr;
    wl_shm* shm = nullptr;
    wl_seat* seat = nullptr;
    xdg_wm_base* xdg_wm_base_ = nullptr;
    zxdg_decoration_manager_v1* decoration_manager = nullptr;
    zwp_text_input_manager_v3* text_input_manager = nullptr;

    int display_fd = -1;
    bool running = false;
    bool connected = false;

    std::vector<uint32_t> shm_formats;

    // Output scale factor (for HiDPI)
    int32_t scale_factor = 1;
    wl_output* primary_output = nullptr;

    OutputAddedCallback output_added_callback;
    OutputRemovedCallback output_removed_callback;
    SeatAddedCallback seat_added_callback;

    // Window tracking for input event routing
    std::unordered_map<wl_surface*, IWaylandWindow*> surface_to_window;

    // Input seat
    WaylandSeat* wayland_seat = nullptr;

public:
    // Wayland callback functions (must be public for C linkage)
    static void registry_global(void* data, wl_registry* registry,
                                uint32_t name, const char* interface, uint32_t version);
    static void registry_global_remove(void* data, wl_registry* registry, uint32_t name);
    static void shm_format(void* data, wl_shm* shm, uint32_t format);
    static void xdg_wm_base_ping(void* data, xdg_wm_base* xdg_wm_base, uint32_t serial);
    static void output_geometry(void* data, wl_output* output, int32_t x, int32_t y,
                                int32_t physical_width, int32_t physical_height,
                                int32_t subpixel, const char* make, const char* model, int32_t transform);
    static void output_mode(void* data, wl_output* output, uint32_t flags,
                            int32_t width, int32_t height, int32_t refresh);
    static void output_done(void* data, wl_output* output);
    static void output_scale(void* data, wl_output* output, int32_t factor);

public:
    WaylandDisplay();
    ~WaylandDisplay();

    bool Connect();
    void Disconnect();
    bool IsConnected() const { return connected; }

    // Event loop
    int GetFd() const { return display_fd; }
    int Dispatch();
    int DispatchPending();
    int Flush();
    int Roundtrip();

    void Run();
    void Stop() { running = false; }
    bool IsRunning() const { return running; }

    // Getters
    wl_display* GetDisplay() const { return display; }
    wl_compositor* GetCompositor() const { return compositor; }
    wl_shm* GetShm() const { return shm; }
    wl_seat* GetSeat() const { return seat; }
    xdg_wm_base* GetXdgWmBase() const { return xdg_wm_base_; }
    zxdg_decoration_manager_v1* GetDecorationManager() const { return decoration_manager; }
    zwp_text_input_manager_v3* GetTextInputManager() const { return text_input_manager; }
    WaylandSeat* GetWaylandSeat() const { return wayland_seat; }

    // Format support
    bool HasShmFormat(uint32_t format) const;

    // Scale factor (for HiDPI)
    int32_t GetOutputScale() const { return scale_factor; }

    // Callbacks
    void SetOutputAddedCallback(OutputAddedCallback cb) { output_added_callback = std::move(cb); }
    void SetOutputRemovedCallback(OutputRemovedCallback cb) { output_removed_callback = std::move(cb); }
    void SetSeatAddedCallback(SeatAddedCallback cb) { seat_added_callback = std::move(cb); }

    // Window registration (for input event routing)
    void RegisterWindow(IWaylandWindow* window);
    void UnregisterWindow(IWaylandWindow* window);
    IWaylandWindow* FindWindowBySurface(wl_surface* surface);
};

// Global instance
WaylandDisplay* GetWaylandDisplay();
void SetWaylandDisplay(WaylandDisplay* display);

} // namespace wayland
} // namespace presentation
} // namespace vl

#endif // WGAC_WAYLAND_DISPLAY_H
