#ifndef WGAC_CURSOR_H
#define WGAC_CURSOR_H

#include "GacUI.h"
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <string>

namespace vl {
namespace presentation {
namespace wayland {

class WaylandDisplay;

class WGacSystemCursor final : public Object, public INativeCursor
{
private:
    SystemCursorType cursorType;

public:
    explicit WGacSystemCursor(SystemCursorType type);

    bool IsSystemCursor() override;
    SystemCursorType GetSystemCursorType() override;
};

class WGacCursorRenderer
{
private:
    WaylandDisplay* display = nullptr;
    wl_shm* shm = nullptr;
    wl_compositor* compositor = nullptr;
    wl_cursor_theme* cursorTheme = nullptr;
    wl_cursor_theme* fallbackTheme = nullptr;
    wl_surface* cursorSurface = nullptr;
    wl_cursor* cursorCache[INativeCursor::SystemCursorCount] = {};
    std::string configuredTheme;
    int logicalCursorSize = 24;
    int loadedScale = 0;

    bool EnsureSurface();
    bool EnsureTheme(int scale);
    void DestroyThemes();
    wl_cursor* FindCursor(
        wl_cursor_theme* theme,
        INativeCursor::SystemCursorType type) const;
    wl_cursor* ResolveCursor(
        INativeCursor::SystemCursorType requestedType);

public:
    explicit WGacCursorRenderer(WaylandDisplay* display);
    ~WGacCursorRenderer();

    bool Apply(
        wl_pointer* pointer,
        uint32_t enterSerial,
        INativeCursor::SystemCursorType type);
    void Destroy();
};

}
}
}

#endif
