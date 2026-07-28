#include "WGacCursor.h"
#include "Wayland/WaylandDisplay.h"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>

namespace vl {
namespace presentation {
namespace wayland {

namespace {
    constexpr int MaxPhysicalCursorSize = 1024;

    int ReadLogicalCursorSize()
    {
        const char* value = std::getenv("XCURSOR_SIZE");
        if (!value || !*value)
        {
            return 24;
        }

        errno = 0;
        char* end = nullptr;
        const long size = std::strtol(value, &end, 10);
        if (errno != 0 || end == value || *end != '\0' ||
            size <= 0 || size > MaxPhysicalCursorSize)
        {
            return 24;
        }
        return static_cast<int>(size);
    }

    template<size_t N>
    wl_cursor* FindFirstCursor(
        wl_cursor_theme* theme,
        const char* const (&names)[N])
    {
        if (!theme)
        {
            return nullptr;
        }

        for (const char* name : names)
        {
            if (auto cursor = wl_cursor_theme_get_cursor(theme, name))
            {
                return cursor;
            }
        }
        return nullptr;
    }
}

WGacSystemCursor::WGacSystemCursor(SystemCursorType type)
    : cursorType(type)
{
}

bool WGacSystemCursor::IsSystemCursor()
{
    return true;
}

INativeCursor::SystemCursorType WGacSystemCursor::GetSystemCursorType()
{
    return cursorType;
}

WGacCursorRenderer::WGacCursorRenderer(WaylandDisplay* _display)
    : display(_display)
    , logicalCursorSize(ReadLogicalCursorSize())
{
    const char* theme = std::getenv("XCURSOR_THEME");
    if (theme && *theme)
    {
        configuredTheme = theme;
    }
}

WGacCursorRenderer::~WGacCursorRenderer()
{
    Destroy();
}

bool WGacCursorRenderer::EnsureSurface()
{
    if (cursorSurface)
    {
        return true;
    }
    if (!display)
    {
        return false;
    }

    shm = display->GetShm();
    compositor = display->GetCompositor();
    if (!shm || !compositor)
    {
        return false;
    }

    cursorSurface = wl_compositor_create_surface(compositor);
    if (!cursorSurface)
    {
        std::fprintf(stderr, "wGac: failed to create the Wayland cursor surface.\n");
        return false;
    }
    return true;
}

bool WGacCursorRenderer::EnsureTheme(int scale)
{
    // libwayland-cursor sizes its initial shared-memory pool with size² * 4.
    // Keep malformed environment values and implausible compositor scales
    // from overflowing that calculation or requesting an excessive pool.
    const int maxScale = std::max(1, MaxPhysicalCursorSize / logicalCursorSize);
    scale = std::clamp(scale, 1, maxScale);
    if (cursorTheme && loadedScale == scale)
    {
        return true;
    }
    if (!EnsureSurface())
    {
        return false;
    }

    const int physicalSize = logicalCursorSize * scale;

    wl_cursor_theme* newTheme = wl_cursor_theme_load(
        configuredTheme.empty() ? nullptr : configuredTheme.c_str(),
        physicalSize,
        shm);
    bool loadedConfiguredTheme = newTheme && !configuredTheme.empty();
    if (!newTheme && !configuredTheme.empty())
    {
        newTheme = wl_cursor_theme_load(nullptr, physicalSize, shm);
        loadedConfiguredTheme = false;
    }
    if (!newTheme)
    {
        std::fprintf(
            stderr,
            "wGac: failed to load a cursor theme at size %d.\n",
            physicalSize);
        return false;
    }

    wl_cursor_theme* newFallbackTheme = nullptr;
    if (loadedConfiguredTheme)
    {
        newFallbackTheme = wl_cursor_theme_load(nullptr, physicalSize, shm);
    }

    if (cursorTheme)
    {
        wl_surface_attach(cursorSurface, nullptr, 0, 0);
        wl_surface_commit(cursorSurface);
    }
    DestroyThemes();
    cursorTheme = newTheme;
    fallbackTheme = newFallbackTheme;
    loadedScale = scale;
    return true;
}

void WGacCursorRenderer::DestroyThemes()
{
    for (auto& cursor : cursorCache)
    {
        cursor = nullptr;
    }
    if (fallbackTheme)
    {
        wl_cursor_theme_destroy(fallbackTheme);
        fallbackTheme = nullptr;
    }
    if (cursorTheme)
    {
        wl_cursor_theme_destroy(cursorTheme);
        cursorTheme = nullptr;
    }
    loadedScale = 0;
}

wl_cursor* WGacCursorRenderer::FindCursor(
    wl_cursor_theme* theme,
    INativeCursor::SystemCursorType type) const
{
    static const char* const smallWaiting[] = {
        "progress",
        "left_ptr_watch",
    };
    static const char* const largeWaiting[] = {
        "wait",
        "watch",
    };
    static const char* const arrow[] = {
        "default",
        "left_ptr",
    };
    static const char* const cross[] = {
        "crosshair",
        "cross",
    };
    static const char* const hand[] = {
        "pointer",
        "hand1",
        "hand2",
    };
    static const char* const help[] = {
        "help",
        "question_arrow",
    };
    static const char* const ibeam[] = {
        "text",
        "xterm",
    };
    static const char* const sizeAll[] = {
        "move",
        "fleur",
        "grabbing",
    };
    static const char* const sizeNesw[] = {
        "nesw-resize",
        "size_bdiag",
        "size-bdiag",
        "ne-resize",
        "fd_double_arrow",
        "fcf1c3c7cd4491d801f1e1c78f100000",
    };
    static const char* const sizeNs[] = {
        "ns-resize",
        "size_ver",
        "size-ver",
        "n-resize",
        "sb_v_double_arrow",
        "v_double_arrow",
    };
    static const char* const sizeNwse[] = {
        "nwse-resize",
        "size_fdiag",
        "size-fdiag",
        "nw-resize",
        "bd_double_arrow",
        "c7088f0f3e6c8088236ef8e1e3e70000",
    };
    static const char* const sizeWe[] = {
        "ew-resize",
        "size_hor",
        "size-hor",
        "e-resize",
        "sb_h_double_arrow",
        "h_double_arrow",
    };

    switch (type)
    {
    case INativeCursor::SmallWaiting:
        return FindFirstCursor(theme, smallWaiting);
    case INativeCursor::LargeWaiting:
        return FindFirstCursor(theme, largeWaiting);
    case INativeCursor::Arrow:
        return FindFirstCursor(theme, arrow);
    case INativeCursor::Cross:
        return FindFirstCursor(theme, cross);
    case INativeCursor::Hand:
        return FindFirstCursor(theme, hand);
    case INativeCursor::Help:
        return FindFirstCursor(theme, help);
    case INativeCursor::IBeam:
        return FindFirstCursor(theme, ibeam);
    case INativeCursor::SizeAll:
        return FindFirstCursor(theme, sizeAll);
    case INativeCursor::SizeNESW:
        return FindFirstCursor(theme, sizeNesw);
    case INativeCursor::SizeNS:
        return FindFirstCursor(theme, sizeNs);
    case INativeCursor::SizeNWSE:
        return FindFirstCursor(theme, sizeNwse);
    case INativeCursor::SizeWE:
        return FindFirstCursor(theme, sizeWe);
    default:
        return nullptr;
    }
}

wl_cursor* WGacCursorRenderer::ResolveCursor(
    INativeCursor::SystemCursorType requestedType)
{
    vint index = static_cast<vint>(requestedType);
    if (index < 0 || index >= INativeCursor::SystemCursorCount)
    {
        requestedType = INativeCursor::Arrow;
        index = static_cast<vint>(requestedType);
    }

    if (cursorCache[index])
    {
        return cursorCache[index];
    }

    auto cursor = FindCursor(cursorTheme, requestedType);
    if (!cursor)
    {
        cursor = FindCursor(fallbackTheme, requestedType);
    }
    if (cursor)
    {
        cursorCache[index] = cursor;
        return cursor;
    }

    const vint arrowIndex = static_cast<vint>(INativeCursor::Arrow);
    cursor = cursorCache[arrowIndex];
    if (!cursor)
    {
        cursor = FindCursor(cursorTheme, INativeCursor::Arrow);
        if (!cursor)
        {
            cursor = FindCursor(fallbackTheme, INativeCursor::Arrow);
        }
        cursorCache[arrowIndex] = cursor;
    }
    return cursor;
}

bool WGacCursorRenderer::Apply(
    wl_pointer* pointer,
    uint32_t enterSerial,
    INativeCursor::SystemCursorType type)
{
    if (!pointer || !display)
    {
        return false;
    }

    int scale = display->GetOutputScale();
    if (scale < 1)
    {
        scale = 1;
    }
    if (!EnsureTheme(scale))
    {
        wl_pointer_set_cursor(pointer, enterSerial, nullptr, 0, 0);
        return false;
    }

    auto cursor = ResolveCursor(type);
    if (!cursor || cursor->image_count == 0)
    {
        std::fprintf(stderr, "wGac: the cursor theme has no Arrow cursor.\n");
        wl_pointer_set_cursor(pointer, enterSerial, nullptr, 0, 0);
        return false;
    }

    // Waiting cursors intentionally use a stable first frame. wGac has no
    // cursor-animation scheduler, so keeping unused animation state would
    // misleadingly suggest that later frames can be displayed.
    wl_cursor_image* image = cursor->images[0];
    wl_buffer* buffer = wl_cursor_image_get_buffer(image);
    if (!buffer)
    {
        std::fprintf(stderr, "wGac: failed to obtain the cursor image buffer.\n");
        wl_pointer_set_cursor(pointer, enterSerial, nullptr, 0, 0);
        return false;
    }

    const int hotspotX = static_cast<int>(image->hotspot_x) / loadedScale;
    const int hotspotY = static_cast<int>(image->hotspot_y) / loadedScale;
    wl_surface_set_buffer_scale(cursorSurface, loadedScale);
    wl_pointer_set_cursor(
        pointer,
        enterSerial,
        cursorSurface,
        hotspotX,
        hotspotY);
    wl_surface_attach(cursorSurface, buffer, 0, 0);
    wl_surface_damage_buffer(
        cursorSurface,
        0,
        0,
        static_cast<int32_t>(image->width),
        static_cast<int32_t>(image->height));
    wl_surface_commit(cursorSurface);

    return true;
}

void WGacCursorRenderer::Destroy()
{
    if (cursorSurface)
    {
        wl_surface_destroy(cursorSurface);
        cursorSurface = nullptr;
    }
    DestroyThemes();
    shm = nullptr;
    compositor = nullptr;
}

}
}
}
