#ifndef WGAC_WAYLAND_SEAT_H
#define WGAC_WAYLAND_SEAT_H

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include "../Protocol/text-input-unstable-v3-client-protocol.h"
#include <functional>
#include <string>
#include <cstdint>

namespace vl {
namespace presentation {
namespace wayland {

class WaylandDisplay;
class IWaylandWindow;

// Mouse button codes
enum class MouseButton {
    Left = 0x110,    // BTN_LEFT
    Right = 0x111,   // BTN_RIGHT
    Middle = 0x112,  // BTN_MIDDLE
};

// Key state
enum class KeyState {
    Released = 0,
    Pressed = 1,
    Repeat = 2
};

// Mouse event info
struct MouseEventInfo {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t button = 0;
    bool left = false;
    bool right = false;
    bool middle = false;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
};

// Keyboard event info
struct KeyEventInfo {
    uint32_t keycode = 0;      // Linux keycode
    uint32_t keysym = 0;       // XKB keysym
    KeyState state = KeyState::Released;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool capsLock = false;
    std::string text;          // UTF-8 text for the key
};

// Scroll event info
struct ScrollEventInfo {
    int32_t x = 0;
    int32_t y = 0;
    double deltaX = 0;
    double deltaY = 0;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
};

// IME preedit info
struct PreeditInfo {
    std::string text;         // UTF-8 preedit text
    int32_t cursorBegin = 0;  // Cursor position in bytes
    int32_t cursorEnd = 0;    // Cursor end position in bytes
};

// Callbacks
using KeyboardEnterCallback = std::function<void(IWaylandWindow*)>;
using KeyboardLeaveCallback = std::function<void(IWaylandWindow*)>;
using KeyEventCallback = std::function<void(IWaylandWindow*, const KeyEventInfo&)>;
using TextInputPreeditCallback = std::function<void(IWaylandWindow*, const PreeditInfo&)>;
using TextInputCommitCallback = std::function<void(IWaylandWindow*, const std::string&)>;

using PointerEnterCallback = std::function<void(IWaylandWindow*, int32_t x, int32_t y)>;
using PointerLeaveCallback = std::function<void(IWaylandWindow*)>;
using PointerMotionCallback = std::function<void(IWaylandWindow*, const MouseEventInfo&)>;
using PointerButtonCallback = std::function<void(IWaylandWindow*, const MouseEventInfo&, bool pressed)>;
using PointerScrollCallback = std::function<void(IWaylandWindow*, const ScrollEventInfo&)>;

class WaylandSeat {
private:
    WaylandDisplay* display = nullptr;
    wl_seat* seat = nullptr;
    wl_keyboard* keyboard = nullptr;
    wl_pointer* pointer = nullptr;
    zwp_text_input_v3* text_input = nullptr;

    // XKB keyboard state
    xkb_context* xkb_ctx = nullptr;
    xkb_keymap* xkb_keymap_ = nullptr;
    xkb_state* xkb_state_ = nullptr;

    // Current focus
    IWaylandWindow* keyboard_focus = nullptr;
    IWaylandWindow* pointer_focus = nullptr;
    wl_surface* pointer_focus_surface = nullptr;

    // Current state
    int32_t pointer_x = 0;
    int32_t pointer_y = 0;
    uint32_t pointer_buttons = 0;
    uint32_t modifiers = 0;
    uint32_t last_pointer_serial = 0;
    uint32_t last_keyboard_serial = 0;

    // Keyboard repeat
    int32_t repeat_rate = 25;   // chars per second
    int32_t repeat_delay = 600; // ms before repeat starts

    // Text input state
    bool text_input_enabled = false;
    PreeditInfo pending_preedit;
    std::string pending_commit;
    bool has_pending_preedit = false;
    bool has_pending_commit = false;

    // Callbacks
    KeyboardEnterCallback keyboard_enter_cb;
    KeyboardLeaveCallback keyboard_leave_cb;
    KeyEventCallback key_event_cb;

    PointerEnterCallback pointer_enter_cb;
    PointerLeaveCallback pointer_leave_cb;
    PointerMotionCallback pointer_motion_cb;
    PointerButtonCallback pointer_button_cb;
    PointerScrollCallback pointer_scroll_cb;
    TextInputPreeditCallback text_input_preedit_cb;
    TextInputCommitCallback text_input_commit_cb;

    // Helper methods
    void UpdateModifiers(uint32_t mods_depressed, uint32_t mods_latched,
                        uint32_t mods_locked, uint32_t group);
    MouseEventInfo CreateMouseEventInfo();
    KeyEventInfo CreateKeyEventInfo(uint32_t key, KeyState state);
    IWaylandWindow* FindWindowBySurface(wl_surface* surface);

public:
    // Seat capability listener
    static void seat_capabilities(void* data, wl_seat* seat, uint32_t caps);
    static void seat_name(void* data, wl_seat* seat, const char* name);

    // Keyboard listeners
    static void keyboard_keymap(void* data, wl_keyboard* keyboard,
                                uint32_t format, int32_t fd, uint32_t size);
    static void keyboard_enter(void* data, wl_keyboard* keyboard,
                               uint32_t serial, wl_surface* surface, wl_array* keys);
    static void keyboard_leave(void* data, wl_keyboard* keyboard,
                               uint32_t serial, wl_surface* surface);
    static void keyboard_key(void* data, wl_keyboard* keyboard,
                             uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    static void keyboard_modifiers(void* data, wl_keyboard* keyboard,
                                   uint32_t serial, uint32_t mods_depressed,
                                   uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
    static void keyboard_repeat_info(void* data, wl_keyboard* keyboard,
                                     int32_t rate, int32_t delay);

    // Pointer listeners
    static void pointer_enter(void* data, wl_pointer* pointer,
                              uint32_t serial, wl_surface* surface,
                              wl_fixed_t sx, wl_fixed_t sy);
    static void pointer_leave(void* data, wl_pointer* pointer,
                              uint32_t serial, wl_surface* surface);
    static void pointer_motion(void* data, wl_pointer* pointer,
                               uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
    static void pointer_button(void* data, wl_pointer* pointer,
                               uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    static void pointer_axis(void* data, wl_pointer* pointer,
                             uint32_t time, uint32_t axis, wl_fixed_t value);
    static void pointer_frame(void* data, wl_pointer* pointer);
    static void pointer_axis_source(void* data, wl_pointer* pointer, uint32_t axis_source);
    static void pointer_axis_stop(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis);
    static void pointer_axis_discrete(void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete);

    // Text input listeners
    static void text_input_enter(void* data, zwp_text_input_v3* text_input, wl_surface* surface);
    static void text_input_leave(void* data, zwp_text_input_v3* text_input, wl_surface* surface);
    static void text_input_preedit_string(void* data, zwp_text_input_v3* text_input,
                                          const char* text, int32_t cursor_begin, int32_t cursor_end);
    static void text_input_commit_string(void* data, zwp_text_input_v3* text_input, const char* text);
    static void text_input_delete_surrounding_text(void* data, zwp_text_input_v3* text_input,
                                                    uint32_t before_length, uint32_t after_length);
    static void text_input_done(void* data, zwp_text_input_v3* text_input, uint32_t serial);

public:
    explicit WaylandSeat(WaylandDisplay* display);
    ~WaylandSeat();

    void Initialize(wl_seat* seat);
    void Destroy();

    // Getters
    wl_seat* GetSeat() const { return seat; }
    wl_pointer* GetPointer() const { return pointer; }
    wl_keyboard* GetKeyboard() const { return keyboard; }

    IWaylandWindow* GetKeyboardFocus() const { return keyboard_focus; }
    IWaylandWindow* GetPointerFocus() const { return pointer_focus; }

    int32_t GetPointerX() const { return pointer_x; }
    int32_t GetPointerY() const { return pointer_y; }
    uint32_t GetLastPointerSerial() const { return last_pointer_serial; }
    uint32_t GetLastKeyboardSerial() const { return last_keyboard_serial; }
    uint32_t GetLastInputSerial() const {
        // Return the most recent serial from any input event
        return last_keyboard_serial > last_pointer_serial ? last_keyboard_serial : last_pointer_serial;
    }

    bool IsModifierPressed(uint32_t mod) const { return (modifiers & mod) != 0; }

    // Cursor
    void SetCursor(wl_surface* cursor_surface, int32_t hotspot_x, int32_t hotspot_y);
    void HideCursor();

    // Text input methods
    void EnableTextInput(wl_surface* surface, int32_t x, int32_t y, int32_t width, int32_t height);
    void DisableTextInput();
    void UpdateCursorRect(int32_t x, int32_t y, int32_t width, int32_t height);
    bool IsTextInputEnabled() const { return text_input_enabled; }

    // Callbacks
    void SetKeyboardEnterCallback(KeyboardEnterCallback cb) { keyboard_enter_cb = std::move(cb); }
    void SetKeyboardLeaveCallback(KeyboardLeaveCallback cb) { keyboard_leave_cb = std::move(cb); }
    void SetKeyEventCallback(KeyEventCallback cb) { key_event_cb = std::move(cb); }

    void SetPointerEnterCallback(PointerEnterCallback cb) { pointer_enter_cb = std::move(cb); }
    void SetPointerLeaveCallback(PointerLeaveCallback cb) { pointer_leave_cb = std::move(cb); }
    void SetPointerMotionCallback(PointerMotionCallback cb) { pointer_motion_cb = std::move(cb); }
    void SetPointerButtonCallback(PointerButtonCallback cb) { pointer_button_cb = std::move(cb); }
    void SetPointerScrollCallback(PointerScrollCallback cb) { pointer_scroll_cb = std::move(cb); }
    void SetTextInputPreeditCallback(TextInputPreeditCallback cb) { text_input_preedit_cb = std::move(cb); }
    void SetTextInputCommitCallback(TextInputCommitCallback cb) { text_input_commit_cb = std::move(cb); }

    // Clear focus references when a window is destroyed
    // If parent is provided, restore pointer focus to parent (for popup dismiss)
    void ClearFocusFor(IWaylandWindow* window, IWaylandWindow* parent = nullptr) {
        if (keyboard_focus == window) {
            keyboard_focus = nullptr;
        }
        if (pointer_focus == window) {
            if (parent) {
                // Restore pointer focus to parent window
                // This is needed because Wayland doesn't send pointer_enter
                // when a popup is dismissed and pointer is already over parent
                pointer_focus = parent;
                // Note: pointer_focus_surface should be updated too, but we don't
                // have easy access to parent's surface here. The next pointer_motion
                // or pointer_enter will fix it.
            } else {
                pointer_focus = nullptr;
                pointer_focus_surface = nullptr;
            }
        }
    }
};

} // namespace wayland
} // namespace presentation
} // namespace vl

#endif // WGAC_WAYLAND_SEAT_H
