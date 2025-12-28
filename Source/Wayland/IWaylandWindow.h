#ifndef IWAYLANDWINDOW_H
#define IWAYLANDWINDOW_H

#include <wayland-client.h>
#include <cstdint>
#include <string>

namespace vl {
namespace presentation {
namespace wayland {

struct MouseEventInfo;
struct ScrollEventInfo;
struct KeyEventInfo;
struct PreeditInfo;

// Interface for windows that receive Wayland input events
class IWaylandWindow {
public:
    virtual ~IWaylandWindow() = default;

    virtual wl_surface* GetSurface() const = 0;

    // Input event handlers
    virtual void OnMouseEnter(int32_t x, int32_t y) = 0;
    virtual void OnMouseLeave() = 0;
    virtual void OnMouseMove(const MouseEventInfo& info) = 0;
    virtual void OnMouseButton(const MouseEventInfo& info, bool pressed) = 0;
    virtual void OnMouseScroll(const ScrollEventInfo& info) = 0;
    virtual void OnKeyEvent(const KeyEventInfo& info) = 0;
    virtual void OnFocusChanged(bool focused) = 0;

    // IME event handlers
    virtual void OnTextInputPreedit(const PreeditInfo& info) = 0;
    virtual void OnTextInputCommit(const std::string& text) = 0;
};

} // namespace wayland
} // namespace presentation
} // namespace vl

#endif // IWAYLANDWINDOW_H
