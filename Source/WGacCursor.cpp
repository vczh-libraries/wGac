#include "WGacCursor.h"
#include "Wayland/WaylandDisplay.h"
#include "Wayland/WaylandSeat.h"
#include <cstdio>
#include <cstdlib>

namespace vl {
namespace presentation {
namespace wayland {

const char* WGacCursor::GetCursorName(CursorType type) {
    switch (type) {
        case CursorType::Arrow:    return "default";
        case CursorType::IBeam:    return "text";
        case CursorType::Wait:     return "wait";
        case CursorType::Cross:    return "crosshair";
        case CursorType::SizeNWSE: return "nwse-resize";
        case CursorType::SizeNESW: return "nesw-resize";
        case CursorType::SizeWE:   return "ew-resize";
        case CursorType::SizeNS:   return "ns-resize";
        case CursorType::SizeAll:  return "move";
        case CursorType::Hand:     return "pointer";
        case CursorType::Help:     return "help";
        case CursorType::No:       return "not-allowed";
        case CursorType::Progress: return "progress";
        default:                   return "default";
    }
}

WGacCursor::WGacCursor(WaylandDisplay* disp)
    : display(disp)
{
}

WGacCursor::~WGacCursor() {
    Destroy();
}

bool WGacCursor::Initialize() {
    if (!display) return false;

    shm = display->GetShm();
    compositor = display->GetCompositor();

    if (!shm || !compositor) {
        return false;
    }

    // Get cursor size from environment or use default
    const char* cursor_size_env = getenv("XCURSOR_SIZE");
    if (cursor_size_env) {
        int size = atoi(cursor_size_env);
        if (size > 0) {
            cursor_size = size;
        }
    }

    // Get cursor theme from environment
    const char* cursor_theme_name = getenv("XCURSOR_THEME");
    if (!cursor_theme_name) {
        cursor_theme_name = "default";
    }

    // Load cursor theme
    cursor_theme = wl_cursor_theme_load(cursor_theme_name, cursor_size, shm);
    if (!cursor_theme) {
        // Try with NULL for system default
        cursor_theme = wl_cursor_theme_load(nullptr, cursor_size, shm);
    }

    if (!cursor_theme) {
        fprintf(stderr, "Failed to load cursor theme\n");
        return false;
    }

    // Create cursor surface
    cursor_surface = wl_compositor_create_surface(compositor);
    if (!cursor_surface) {
        fprintf(stderr, "Failed to create cursor surface\n");
        wl_cursor_theme_destroy(cursor_theme);
        cursor_theme = nullptr;
        return false;
    }

    printf("Cursor theme loaded: %s (size: %d)\n", cursor_theme_name, cursor_size);
    return true;
}

void WGacCursor::Destroy() {
    cursor_cache.clear();
    current_cursor = nullptr;

    if (cursor_surface) {
        wl_surface_destroy(cursor_surface);
        cursor_surface = nullptr;
    }

    if (cursor_theme) {
        wl_cursor_theme_destroy(cursor_theme);
        cursor_theme = nullptr;
    }
}

bool WGacCursor::SetCursor(WaylandSeat* seat, CursorType type) {
    if (!cursor_theme || !cursor_surface || !seat) {
        return false;
    }

    // Check cache first
    auto it = cursor_cache.find(type);
    wl_cursor* cursor = nullptr;

    if (it != cursor_cache.end()) {
        cursor = it->second;
    } else {
        // Load cursor
        const char* cursor_name = GetCursorName(type);
        cursor = wl_cursor_theme_get_cursor(cursor_theme, cursor_name);

        // Try fallback names
        if (!cursor) {
            switch (type) {
                case CursorType::IBeam:
                    cursor = wl_cursor_theme_get_cursor(cursor_theme, "xterm");
                    break;
                case CursorType::Hand:
                    cursor = wl_cursor_theme_get_cursor(cursor_theme, "hand1");
                    if (!cursor) cursor = wl_cursor_theme_get_cursor(cursor_theme, "hand2");
                    break;
                case CursorType::SizeNWSE:
                    cursor = wl_cursor_theme_get_cursor(cursor_theme, "size_fdiag");
                    if (!cursor) cursor = wl_cursor_theme_get_cursor(cursor_theme, "nw-resize");
                    break;
                case CursorType::SizeNESW:
                    cursor = wl_cursor_theme_get_cursor(cursor_theme, "size_bdiag");
                    if (!cursor) cursor = wl_cursor_theme_get_cursor(cursor_theme, "ne-resize");
                    break;
                case CursorType::SizeWE:
                    cursor = wl_cursor_theme_get_cursor(cursor_theme, "size_hor");
                    if (!cursor) cursor = wl_cursor_theme_get_cursor(cursor_theme, "e-resize");
                    break;
                case CursorType::SizeNS:
                    cursor = wl_cursor_theme_get_cursor(cursor_theme, "size_ver");
                    if (!cursor) cursor = wl_cursor_theme_get_cursor(cursor_theme, "n-resize");
                    break;
                case CursorType::SizeAll:
                    cursor = wl_cursor_theme_get_cursor(cursor_theme, "fleur");
                    if (!cursor) cursor = wl_cursor_theme_get_cursor(cursor_theme, "grabbing");
                    break;
                case CursorType::No:
                    cursor = wl_cursor_theme_get_cursor(cursor_theme, "crossed_circle");
                    if (!cursor) cursor = wl_cursor_theme_get_cursor(cursor_theme, "forbidden");
                    break;
                case CursorType::Wait:
                    cursor = wl_cursor_theme_get_cursor(cursor_theme, "watch");
                    break;
                case CursorType::Progress:
                    cursor = wl_cursor_theme_get_cursor(cursor_theme, "left_ptr_watch");
                    break;
                default:
                    break;
            }
        }

        // Fall back to default cursor
        if (!cursor) {
            cursor = wl_cursor_theme_get_cursor(cursor_theme, "default");
            if (!cursor) {
                cursor = wl_cursor_theme_get_cursor(cursor_theme, "left_ptr");
            }
        }

        if (cursor) {
            cursor_cache[type] = cursor;
        }
    }

    if (!cursor || cursor->image_count == 0) {
        return false;
    }

    current_cursor = cursor;
    current_type = type;
    current_image_index = 0;

    // Get first image
    wl_cursor_image* image = cursor->images[0];
    wl_buffer* buffer = wl_cursor_image_get_buffer(image);

    if (!buffer) {
        return false;
    }

    // Update cursor surface
    wl_surface_attach(cursor_surface, buffer, 0, 0);
    wl_surface_damage(cursor_surface, 0, 0, image->width, image->height);
    wl_surface_commit(cursor_surface);

    // Set cursor on seat
    seat->SetCursor(cursor_surface, image->hotspot_x, image->hotspot_y);

    return true;
}

void WGacCursor::HideCursor(WaylandSeat* seat) {
    if (seat) {
        seat->HideCursor();
    }
    current_cursor = nullptr;
}

int WGacCursor::GetHotspotX() const {
    if (current_cursor && current_cursor->image_count > 0) {
        return current_cursor->images[current_image_index]->hotspot_x;
    }
    return 0;
}

int WGacCursor::GetHotspotY() const {
    if (current_cursor && current_cursor->image_count > 0) {
        return current_cursor->images[current_image_index]->hotspot_y;
    }
    return 0;
}

void WGacCursor::UpdateAnimation(WaylandSeat* seat, uint32_t time) {
    if (!current_cursor || current_cursor->image_count <= 1) {
        return;
    }

    // Calculate which frame to show
    uint32_t duration = 0;
    for (size_t i = 0; i < current_cursor->image_count; ++i) {
        duration += current_cursor->images[i]->delay;
    }

    if (duration == 0) {
        return;
    }

    uint32_t animation_time = time % duration;
    uint32_t accumulated = 0;
    int new_index = 0;

    for (size_t i = 0; i < current_cursor->image_count; ++i) {
        accumulated += current_cursor->images[i]->delay;
        if (animation_time < accumulated) {
            new_index = i;
            break;
        }
    }

    if (new_index != current_image_index) {
        current_image_index = new_index;

        wl_cursor_image* image = current_cursor->images[current_image_index];
        wl_buffer* buffer = wl_cursor_image_get_buffer(image);

        if (buffer) {
            wl_surface_attach(cursor_surface, buffer, 0, 0);
            wl_surface_damage(cursor_surface, 0, 0, image->width, image->height);
            wl_surface_commit(cursor_surface);

            if (seat) {
                seat->SetCursor(cursor_surface, image->hotspot_x, image->hotspot_y);
            }
        }
    }
}

bool WGacCursor::IsAnimated() const {
    return current_cursor && current_cursor->image_count > 1;
}

} // namespace wayland
} // namespace presentation
} // namespace vl
