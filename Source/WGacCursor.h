#ifndef WGAC_CURSOR_H
#define WGAC_CURSOR_H

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <string>
#include <unordered_map>

namespace vl {
namespace presentation {
namespace wayland {

class WaylandDisplay;
class WaylandSeat;

// Cursor types matching common system cursors
enum class CursorType {
    Arrow,
    IBeam,
    Wait,
    Cross,
    SizeNWSE,   // Diagonal resize NW-SE
    SizeNESW,   // Diagonal resize NE-SW
    SizeWE,     // Horizontal resize
    SizeNS,     // Vertical resize
    SizeAll,    // Move/drag
    Hand,       // Pointer/link
    Help,
    No,         // Not allowed
    Progress,   // Arrow + wait
    Default = Arrow
};

class WGacCursor {
private:
    WaylandDisplay* display = nullptr;
    wl_shm* shm = nullptr;
    wl_compositor* compositor = nullptr;

    // Cursor theme
    wl_cursor_theme* cursor_theme = nullptr;
    int cursor_size = 24;

    // Cache of loaded cursors
    std::unordered_map<CursorType, wl_cursor*> cursor_cache;

    // Current cursor surface
    wl_surface* cursor_surface = nullptr;
    wl_cursor* current_cursor = nullptr;
    CursorType current_type = CursorType::Arrow;

    // Animation
    int current_image_index = 0;
    uint32_t animation_callback_time = 0;

    // Map cursor type to Wayland cursor name
    static const char* GetCursorName(CursorType type);

public:
    explicit WGacCursor(WaylandDisplay* display);
    ~WGacCursor();

    bool Initialize();
    void Destroy();

    // Set cursor
    bool SetCursor(WaylandSeat* seat, CursorType type);
    void HideCursor(WaylandSeat* seat);

    // Get current cursor info for animation
    wl_surface* GetCursorSurface() const { return cursor_surface; }
    int GetHotspotX() const;
    int GetHotspotY() const;

    // Animation update (call from frame callback if cursor is animated)
    void UpdateAnimation(WaylandSeat* seat, uint32_t time);

    // Check if current cursor is animated
    bool IsAnimated() const;
};

} // namespace wayland
} // namespace presentation
} // namespace vl

#endif // WGAC_CURSOR_H
