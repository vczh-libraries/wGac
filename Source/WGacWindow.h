#ifndef WGAC_WINDOW_H
#define WGAC_WINDOW_H

#include "Wayland/WaylandDisplay.h"
#include "Wayland/WaylandBuffer.h"
#include "Wayland/WaylandSeat.h"
#include "Wayland/IWaylandWindow.h"
#include <string>
#include <vector>
#include <functional>

namespace vl {
namespace presentation {
namespace wayland {

class WGacWindowView;

enum class WindowMode {
    Normal,
    Popup,
    Menu,
    Tooltip
};

enum class WindowSizeState {
    Restored,
    Minimized,
    Maximized
};

struct WindowConfig {
    std::string title = "wGac Window";
    int32_t width = 800;
    int32_t height = 600;
    int32_t min_width = 100;
    int32_t min_height = 100;
    bool resizable = true;
    bool decorated = true;
    WindowMode mode = WindowMode::Normal;
};

class WGacWindow : public IWaylandWindow {
public:
    using CloseCallback = std::function<void()>;
    using ResizeCallback = std::function<void(int32_t width, int32_t height)>;
    using FrameCallback = std::function<void()>;

    // Input event callbacks
    using MouseMoveCallback = std::function<void(const MouseEventInfo&)>;
    using MouseButtonCallback = std::function<void(const MouseEventInfo&, bool pressed)>;
    using MouseScrollCallback = std::function<void(const ScrollEventInfo&)>;
    using MouseEnterCallback = std::function<void(int32_t x, int32_t y)>;
    using MouseLeaveCallback = std::function<void()>;
    using KeyboardCallback = std::function<void(const KeyEventInfo&)>;
    using FocusCallback = std::function<void(bool focused)>;

private:
    WaylandDisplay* display = nullptr;

    wl_surface* surface = nullptr;
    xdg_surface* xdg_surface_ = nullptr;
    xdg_toplevel* toplevel = nullptr;
    zxdg_toplevel_decoration_v1* decoration = nullptr;

    wl_callback* frame_callback = nullptr;

    WaylandBufferPool* buffer_pool = nullptr;
    WGacWindowView* view = nullptr;

    WindowConfig config;

    int32_t current_width = 0;
    int32_t current_height = 0;
    bool configured = false;
    bool visible = false;
    bool closed = false;
    bool pending_frame = false;
    bool has_first_frame = false;

    WindowSizeState size_state = WindowSizeState::Restored;

    CloseCallback close_callback;
    ResizeCallback resize_callback;
    FrameCallback frame_handler;

    // Input callbacks
    MouseMoveCallback mouse_move_callback;
    MouseButtonCallback mouse_button_callback;
    MouseScrollCallback mouse_scroll_callback;
    MouseEnterCallback mouse_enter_callback;
    MouseLeaveCallback mouse_leave_callback;
    KeyboardCallback keyboard_callback;
    FocusCallback focus_callback;

    void RequestFrame();
    void OnFrame();

public:
    // Wayland callbacks (must be public for C linkage)
    static void xdg_surface_configure(void* data, xdg_surface* xdg_surface, uint32_t serial);
    static void xdg_toplevel_configure(void* data, xdg_toplevel* toplevel,
                                        int32_t width, int32_t height, wl_array* states);
    static void xdg_toplevel_close(void* data, xdg_toplevel* toplevel);
    static void xdg_toplevel_configure_bounds(void* data, xdg_toplevel* toplevel,
                                               int32_t width, int32_t height);
    static void xdg_toplevel_wm_capabilities(void* data, xdg_toplevel* toplevel,
                                              wl_array* capabilities);
    static void frame_done(void* data, wl_callback* callback, uint32_t time);

public:
    explicit WGacWindow(WaylandDisplay* display);
    ~WGacWindow();

    // No copy
    WGacWindow(const WGacWindow&) = delete;
    WGacWindow& operator=(const WGacWindow&) = delete;

    bool Create(const WindowConfig& cfg = WindowConfig());
    void Destroy();

    // Visibility
    void Show();
    void Hide();
    bool IsVisible() const { return visible; }

    // Size and position
    void SetSize(int32_t width, int32_t height);
    int32_t GetWidth() const { return current_width; }
    int32_t GetHeight() const { return current_height; }

    void SetMinSize(int32_t width, int32_t height);
    void SetMaxSize(int32_t width, int32_t height);

    // Title
    void SetTitle(const std::string& title);
    const std::string& GetTitle() const { return config.title; }

    // State
    void Maximize();
    void Minimize();
    void Restore();
    WindowSizeState GetSizeState() const { return size_state; }

    bool IsConfigured() const { return configured; }
    bool IsClosed() const { return closed; }

    // View/Rendering
    WGacWindowView* GetView() const { return view; }
    void Invalidate();  // Request redraw

    // Commit changes to compositor
    void Commit();

    // Get Wayland objects
    wl_surface* GetSurface() const override { return surface; }
    xdg_toplevel* GetToplevel() const { return toplevel; }

    // Callbacks
    void SetCloseCallback(CloseCallback cb) { close_callback = std::move(cb); }
    void SetResizeCallback(ResizeCallback cb) { resize_callback = std::move(cb); }
    void SetFrameHandler(FrameCallback cb) { frame_handler = std::move(cb); }

    // Input callbacks
    void SetMouseMoveCallback(MouseMoveCallback cb) { mouse_move_callback = std::move(cb); }
    void SetMouseButtonCallback(MouseButtonCallback cb) { mouse_button_callback = std::move(cb); }
    void SetMouseScrollCallback(MouseScrollCallback cb) { mouse_scroll_callback = std::move(cb); }
    void SetMouseEnterCallback(MouseEnterCallback cb) { mouse_enter_callback = std::move(cb); }
    void SetMouseLeaveCallback(MouseLeaveCallback cb) { mouse_leave_callback = std::move(cb); }
    void SetKeyboardCallback(KeyboardCallback cb) { keyboard_callback = std::move(cb); }
    void SetFocusCallback(FocusCallback cb) { focus_callback = std::move(cb); }

    // Input event handlers (called by WaylandSeat) - IWaylandWindow implementation
    void OnMouseMove(const MouseEventInfo& info) override;
    void OnMouseButton(const MouseEventInfo& info, bool pressed) override;
    void OnMouseScroll(const ScrollEventInfo& info) override;
    void OnMouseEnter(int32_t x, int32_t y) override;
    void OnMouseLeave() override;
    void OnKeyEvent(const KeyEventInfo& info) override;
    void OnFocusChanged(bool focused) override;
};

} // namespace wayland
} // namespace presentation
} // namespace vl

#endif // WGAC_WINDOW_H
