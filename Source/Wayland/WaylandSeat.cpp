#include "WaylandSeat.h"
#include "WaylandDisplay.h"
#include "IWaylandWindow.h"
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

namespace vl {
namespace presentation {
namespace wayland {

// Wayland seat listener
static const wl_seat_listener seat_listener = {
    WaylandSeat::seat_capabilities,
    WaylandSeat::seat_name
};

// Wayland keyboard listener
static const wl_keyboard_listener keyboard_listener = {
    WaylandSeat::keyboard_keymap,
    WaylandSeat::keyboard_enter,
    WaylandSeat::keyboard_leave,
    WaylandSeat::keyboard_key,
    WaylandSeat::keyboard_modifiers,
    WaylandSeat::keyboard_repeat_info
};

// Wayland pointer listener
static const wl_pointer_listener pointer_listener = {
    WaylandSeat::pointer_enter,
    WaylandSeat::pointer_leave,
    WaylandSeat::pointer_motion,
    WaylandSeat::pointer_button,
    WaylandSeat::pointer_axis,
    WaylandSeat::pointer_frame,
    WaylandSeat::pointer_axis_source,
    WaylandSeat::pointer_axis_stop,
    WaylandSeat::pointer_axis_discrete,
    nullptr,  // axis_value120 (Wayland 1.21+)
    nullptr   // axis_relative_direction (Wayland 1.22+)
};

// Text input listener
static const zwp_text_input_v3_listener text_input_listener = {
    WaylandSeat::text_input_enter,
    WaylandSeat::text_input_leave,
    WaylandSeat::text_input_preedit_string,
    WaylandSeat::text_input_commit_string,
    WaylandSeat::text_input_delete_surrounding_text,
    WaylandSeat::text_input_done,
};

WaylandSeat::WaylandSeat(WaylandDisplay* disp)
    : display(disp)
{
    // Create XKB context
    xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!xkb_ctx) {
        fprintf(stderr, "Failed to create XKB context\n");
    }
}

WaylandSeat::~WaylandSeat() {
    Destroy();
}

void WaylandSeat::Initialize(wl_seat* s) {
    seat = s;
    wl_seat_add_listener(seat, &seat_listener, this);
}

void WaylandSeat::Destroy() {
    if (text_input) {
        zwp_text_input_v3_destroy(text_input);
        text_input = nullptr;
    }

    if (xkb_state_) {
        xkb_state_unref(xkb_state_);
        xkb_state_ = nullptr;
    }

    if (xkb_keymap_) {
        xkb_keymap_unref(xkb_keymap_);
        xkb_keymap_ = nullptr;
    }

    if (xkb_ctx) {
        xkb_context_unref(xkb_ctx);
        xkb_ctx = nullptr;
    }

    if (keyboard) {
        wl_keyboard_destroy(keyboard);
        keyboard = nullptr;
    }

    if (pointer) {
        wl_pointer_destroy(pointer);
        pointer = nullptr;
    }

    // Note: We don't destroy 'seat' here because it's owned by WaylandDisplay
    seat = nullptr;
}

// Seat capability callback
void WaylandSeat::seat_capabilities(void* data, wl_seat* seat, uint32_t caps) {
    auto* self = static_cast<WaylandSeat*>(data);

    // Handle keyboard capability
    bool has_keyboard = caps & WL_SEAT_CAPABILITY_KEYBOARD;
    if (has_keyboard && !self->keyboard) {
        self->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(self->keyboard, &keyboard_listener, self);
        printf("Keyboard capability acquired\n");

        // Create text input if manager is available
        auto* text_input_manager = self->display->GetTextInputManager();
        if (text_input_manager && !self->text_input) {
            self->text_input = zwp_text_input_manager_v3_get_text_input(text_input_manager, seat);
            zwp_text_input_v3_add_listener(self->text_input, &text_input_listener, self);
            printf("Text input created\n");
        }
    } else if (!has_keyboard && self->keyboard) {
        if (self->text_input) {
            zwp_text_input_v3_destroy(self->text_input);
            self->text_input = nullptr;
        }
        wl_keyboard_destroy(self->keyboard);
        self->keyboard = nullptr;
        printf("Keyboard capability lost\n");
    }

    // Handle pointer capability
    bool has_pointer = caps & WL_SEAT_CAPABILITY_POINTER;
    if (has_pointer && !self->pointer) {
        self->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(self->pointer, &pointer_listener, self);
        printf("Pointer capability acquired\n");
    } else if (!has_pointer && self->pointer) {
        wl_pointer_destroy(self->pointer);
        self->pointer = nullptr;
        printf("Pointer capability lost\n");
    }
}

void WaylandSeat::seat_name(void* data, wl_seat* seat, const char* name) {
    (void)data;
    (void)seat;
    printf("Seat name: %s\n", name);
}

// Keyboard callbacks
void WaylandSeat::keyboard_keymap(void* data, wl_keyboard* keyboard,
                                   uint32_t format, int32_t fd, uint32_t size) {
    (void)keyboard;
    auto* self = static_cast<WaylandSeat*>(data);

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        fprintf(stderr, "Unsupported keymap format\n");
        close(fd);
        return;
    }

    // Map the keymap
    char* map_shm = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (map_shm == MAP_FAILED) {
        fprintf(stderr, "Failed to mmap keymap\n");
        close(fd);
        return;
    }

    // Create XKB keymap
    if (self->xkb_keymap_) {
        xkb_keymap_unref(self->xkb_keymap_);
    }
    self->xkb_keymap_ = xkb_keymap_new_from_string(
        self->xkb_ctx, map_shm,
        XKB_KEYMAP_FORMAT_TEXT_V1,
        XKB_KEYMAP_COMPILE_NO_FLAGS
    );

    munmap(map_shm, size);
    close(fd);

    if (!self->xkb_keymap_) {
        fprintf(stderr, "Failed to compile keymap\n");
        return;
    }

    // Create XKB state
    if (self->xkb_state_) {
        xkb_state_unref(self->xkb_state_);
    }
    self->xkb_state_ = xkb_state_new(self->xkb_keymap_);

    if (!self->xkb_state_) {
        fprintf(stderr, "Failed to create XKB state\n");
        return;
    }

    printf("Keymap loaded successfully\n");
}

void WaylandSeat::keyboard_enter(void* data, wl_keyboard* keyboard,
                                  uint32_t serial, wl_surface* surface, wl_array* keys) {
    (void)keyboard;
    (void)serial;
    (void)keys;
    auto* self = static_cast<WaylandSeat*>(data);

    self->keyboard_focus = self->FindWindowBySurface(surface);
    if (self->keyboard_focus && self->keyboard_enter_cb) {
        self->keyboard_enter_cb(self->keyboard_focus);
    }
    printf("Keyboard enter\n");
}

void WaylandSeat::keyboard_leave(void* data, wl_keyboard* keyboard,
                                  uint32_t serial, wl_surface* surface) {
    (void)keyboard;
    (void)serial;
    (void)surface;
    auto* self = static_cast<WaylandSeat*>(data);

    if (self->keyboard_focus && self->keyboard_leave_cb) {
        self->keyboard_leave_cb(self->keyboard_focus);
    }
    self->keyboard_focus = nullptr;
    printf("Keyboard leave\n");
}

void WaylandSeat::keyboard_key(void* data, wl_keyboard* keyboard,
                                uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    (void)keyboard;
    (void)serial;
    (void)time;
    auto* self = static_cast<WaylandSeat*>(data);

    if (!self->keyboard_focus) return;

    KeyState keyState = (state == WL_KEYBOARD_KEY_STATE_PRESSED)
        ? KeyState::Pressed
        : KeyState::Released;

    KeyEventInfo info = self->CreateKeyEventInfo(key, keyState);

    if (self->key_event_cb) {
        self->key_event_cb(self->keyboard_focus, info);
    }
}

void WaylandSeat::keyboard_modifiers(void* data, wl_keyboard* keyboard,
                                      uint32_t serial, uint32_t mods_depressed,
                                      uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
    (void)keyboard;
    (void)serial;
    auto* self = static_cast<WaylandSeat*>(data);

    self->UpdateModifiers(mods_depressed, mods_latched, mods_locked, group);
}

void WaylandSeat::keyboard_repeat_info(void* data, wl_keyboard* keyboard,
                                        int32_t rate, int32_t delay) {
    (void)keyboard;
    auto* self = static_cast<WaylandSeat*>(data);

    self->repeat_rate = rate;
    self->repeat_delay = delay;
    printf("Keyboard repeat: rate=%d delay=%d\n", rate, delay);
}

// Pointer callbacks
void WaylandSeat::pointer_enter(void* data, wl_pointer* pointer,
                                 uint32_t serial, wl_surface* surface,
                                 wl_fixed_t sx, wl_fixed_t sy) {
    (void)pointer;
    auto* self = static_cast<WaylandSeat*>(data);

    self->last_pointer_serial = serial;
    self->pointer_focus_surface = surface;
    self->pointer_focus = self->FindWindowBySurface(surface);
    self->pointer_x = wl_fixed_to_int(sx);
    self->pointer_y = wl_fixed_to_int(sy);

    printf("pointer_enter: surface=%p window=%p\n", surface, self->pointer_focus);

    if (self->pointer_focus && self->pointer_enter_cb) {
        self->pointer_enter_cb(self->pointer_focus, self->pointer_x, self->pointer_y);
    }
}

void WaylandSeat::pointer_leave(void* data, wl_pointer* pointer,
                                 uint32_t serial, wl_surface* surface) {
    (void)pointer;
    (void)serial;
    auto* self = static_cast<WaylandSeat*>(data);

    printf("pointer_leave: surface=%p window=%p\n", surface, self->pointer_focus);

    if (self->pointer_focus && self->pointer_leave_cb) {
        self->pointer_leave_cb(self->pointer_focus);
    }
    self->pointer_focus = nullptr;
    self->pointer_focus_surface = nullptr;
}

void WaylandSeat::pointer_motion(void* data, wl_pointer* pointer,
                                  uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
    (void)pointer;
    (void)time;
    auto* self = static_cast<WaylandSeat*>(data);

    self->pointer_x = wl_fixed_to_int(sx);
    self->pointer_y = wl_fixed_to_int(sy);

    if (self->pointer_focus && self->pointer_motion_cb) {
        MouseEventInfo info = self->CreateMouseEventInfo();
        self->pointer_motion_cb(self->pointer_focus, info);
    }
}

void WaylandSeat::pointer_button(void* data, wl_pointer* pointer,
                                  uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    (void)pointer;
    (void)time;
    auto* self = static_cast<WaylandSeat*>(data);

    self->last_pointer_serial = serial;

    bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);

    // Update button state
    if (pressed) {
        self->pointer_buttons |= (1 << (button - 0x110));
    } else {
        self->pointer_buttons &= ~(1 << (button - 0x110));
    }

    if (self->pointer_focus && self->pointer_button_cb) {
        MouseEventInfo info = self->CreateMouseEventInfo();
        info.button = button;
        self->pointer_button_cb(self->pointer_focus, info, pressed);
    }
}

void WaylandSeat::pointer_axis(void* data, wl_pointer* pointer,
                                uint32_t time, uint32_t axis, wl_fixed_t value) {
    (void)pointer;
    (void)time;
    auto* self = static_cast<WaylandSeat*>(data);

    if (self->pointer_focus && self->pointer_scroll_cb) {
        ScrollEventInfo info;
        info.x = self->pointer_x;
        info.y = self->pointer_y;

        // Check modifier states using XKB
        if (self->xkb_state_) {
            info.ctrl = xkb_state_mod_name_is_active(self->xkb_state_, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0;
            info.shift = xkb_state_mod_name_is_active(self->xkb_state_, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0;
            info.alt = xkb_state_mod_name_is_active(self->xkb_state_, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0;
        }

        double delta = wl_fixed_to_double(value);
        if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
            info.deltaY = delta;
        } else {
            info.deltaX = delta;
        }

        self->pointer_scroll_cb(self->pointer_focus, info);
    }
}

void WaylandSeat::pointer_frame(void* data, wl_pointer* pointer) {
    (void)data;
    (void)pointer;
    // Frame callback - all events in a single frame have been received
}

void WaylandSeat::pointer_axis_source(void* data, wl_pointer* pointer, uint32_t axis_source) {
    (void)data;
    (void)pointer;
    (void)axis_source;
}

void WaylandSeat::pointer_axis_stop(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis) {
    (void)data;
    (void)pointer;
    (void)time;
    (void)axis;
}

void WaylandSeat::pointer_axis_discrete(void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete) {
    (void)data;
    (void)pointer;
    (void)axis;
    (void)discrete;
}

// Helper methods
void WaylandSeat::UpdateModifiers(uint32_t mods_depressed, uint32_t mods_latched,
                                   uint32_t mods_locked, uint32_t group) {
    if (!xkb_state_) return;

    xkb_state_update_mask(xkb_state_,
                          mods_depressed, mods_latched, mods_locked,
                          0, 0, group);

    modifiers = mods_depressed | mods_latched;
}

MouseEventInfo WaylandSeat::CreateMouseEventInfo() {
    MouseEventInfo info;
    info.x = pointer_x;
    info.y = pointer_y;

    // Check button states
    info.left = (pointer_buttons & (1 << 0)) != 0;
    info.right = (pointer_buttons & (1 << 1)) != 0;
    info.middle = (pointer_buttons & (1 << 2)) != 0;

    // Check modifier states using XKB
    if (xkb_state_) {
        info.ctrl = xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0;
        info.shift = xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0;
        info.alt = xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0;
    }

    return info;
}

KeyEventInfo WaylandSeat::CreateKeyEventInfo(uint32_t key, KeyState state) {
    KeyEventInfo info;
    info.keycode = key;
    info.state = state;

    if (xkb_state_ && xkb_keymap_) {
        // XKB keycodes are offset by 8 from Linux keycodes
        xkb_keycode_t xkb_key = key + 8;

        // Get keysym
        info.keysym = xkb_state_key_get_one_sym(xkb_state_, xkb_key);

        // Get UTF-8 text for the key
        char buf[32];
        int len = xkb_state_key_get_utf8(xkb_state_, xkb_key, buf, sizeof(buf));
        if (len > 0 && len < (int)sizeof(buf)) {
            // Only include printable characters
            if ((unsigned char)buf[0] >= 0x20) {
                info.text = std::string(buf, len);
            }
        }

        // Check modifiers
        info.ctrl = xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0;
        info.shift = xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0;
        info.alt = xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0;
        info.capsLock = xkb_state_mod_name_is_active(xkb_state_, XKB_MOD_NAME_CAPS, XKB_STATE_MODS_LOCKED) > 0;
    }

    return info;
}

IWaylandWindow* WaylandSeat::FindWindowBySurface(wl_surface* surface) {
    if (!display) return nullptr;
    return display->FindWindowBySurface(surface);
}

// Cursor methods
void WaylandSeat::SetCursor(wl_surface* cursor_surface, int32_t hotspot_x, int32_t hotspot_y) {
    if (pointer && pointer_focus_surface) {
        wl_pointer_set_cursor(pointer, last_pointer_serial, cursor_surface, hotspot_x, hotspot_y);
    }
}

void WaylandSeat::HideCursor() {
    if (pointer && pointer_focus_surface) {
        wl_pointer_set_cursor(pointer, last_pointer_serial, nullptr, 0, 0);
    }
}

// Text input callbacks
void WaylandSeat::text_input_enter(void* data, zwp_text_input_v3* /*text_input*/, wl_surface* surface) {
    auto* self = static_cast<WaylandSeat*>(data);
    (void)surface;
    printf("Text input enter\n");
    // Text input is active for this surface
}

void WaylandSeat::text_input_leave(void* data, zwp_text_input_v3* /*text_input*/, wl_surface* surface) {
    auto* self = static_cast<WaylandSeat*>(data);
    (void)surface;
    printf("Text input leave\n");
    // Clear any pending state
    self->pending_preedit = PreeditInfo{};
    self->pending_commit.clear();
    self->has_pending_preedit = false;
    self->has_pending_commit = false;
}

void WaylandSeat::text_input_preedit_string(void* data, zwp_text_input_v3* /*text_input*/,
                                            const char* text, int32_t cursor_begin, int32_t cursor_end) {
    auto* self = static_cast<WaylandSeat*>(data);
    // Store pending preedit - will be applied on done event
    self->pending_preedit.text = text ? text : "";
    self->pending_preedit.cursorBegin = cursor_begin;
    self->pending_preedit.cursorEnd = cursor_end;
    self->has_pending_preedit = true;
}

void WaylandSeat::text_input_commit_string(void* data, zwp_text_input_v3* /*text_input*/, const char* text) {
    auto* self = static_cast<WaylandSeat*>(data);
    // Store pending commit - will be applied on done event
    self->pending_commit = text ? text : "";
    self->has_pending_commit = true;
}

void WaylandSeat::text_input_delete_surrounding_text(void* data, zwp_text_input_v3* /*text_input*/,
                                                      uint32_t before_length, uint32_t after_length) {
    auto* self = static_cast<WaylandSeat*>(data);
    // TODO: Handle delete surrounding text for editing
    (void)self;
    (void)before_length;
    (void)after_length;
}

void WaylandSeat::text_input_done(void* data, zwp_text_input_v3* /*text_input*/, uint32_t serial) {
    auto* self = static_cast<WaylandSeat*>(data);
    (void)serial;

    // Process pending events
    if (self->has_pending_commit && self->keyboard_focus) {
        // Commit string takes precedence - send the final text
        if (self->text_input_commit_cb) {
            self->text_input_commit_cb(self->keyboard_focus, self->pending_commit);
        }
        // Clear preedit when commit happens
        if (self->text_input_preedit_cb) {
            PreeditInfo empty;
            self->text_input_preedit_cb(self->keyboard_focus, empty);
        }
    } else if (self->has_pending_preedit && self->keyboard_focus) {
        // Update preedit display
        if (self->text_input_preedit_cb) {
            self->text_input_preedit_cb(self->keyboard_focus, self->pending_preedit);
        }
    }

    // Clear pending state
    self->pending_preedit = PreeditInfo{};
    self->pending_commit.clear();
    self->has_pending_preedit = false;
    self->has_pending_commit = false;
}

// Text input methods
void WaylandSeat::EnableTextInput(wl_surface* surface, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (!text_input) return;

    text_input_enabled = true;

    // Enable text input
    zwp_text_input_v3_enable(text_input);

    // Set content type for general text input
    zwp_text_input_v3_set_content_type(text_input,
        ZWP_TEXT_INPUT_V3_CONTENT_HINT_NONE,
        ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NORMAL);

    // Set cursor rectangle (for IME candidate window positioning)
    zwp_text_input_v3_set_cursor_rectangle(text_input, x, y, width, height);

    // Commit the changes
    zwp_text_input_v3_commit(text_input);

    printf("Text input enabled at (%d, %d, %d, %d)\n", x, y, width, height);
}

void WaylandSeat::DisableTextInput() {
    if (!text_input) return;

    text_input_enabled = false;
    zwp_text_input_v3_disable(text_input);
    zwp_text_input_v3_commit(text_input);

    printf("Text input disabled\n");
}

void WaylandSeat::UpdateCursorRect(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (!text_input || !text_input_enabled) return;

    zwp_text_input_v3_set_cursor_rectangle(text_input, x, y, width, height);
    zwp_text_input_v3_commit(text_input);
}


} // namespace wayland
} // namespace presentation
} // namespace vl
