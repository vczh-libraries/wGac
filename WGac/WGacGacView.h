#ifndef WGAC_GAC_VIEW_H
#define WGAC_GAC_VIEW_H

#include "Wayland/WaylandBuffer.h"
#include <cairo/cairo.h>

namespace vl {
namespace presentation {
namespace wayland {

class WGacNativeWindow;

class WGacView
{
protected:
    WGacNativeWindow* window;
    WaylandBufferPool* bufferPool;
    WaylandBuffer* currentBuffer;
    cairo_t* cairoContext;
    bool rendering;

public:
    WGacView(WGacNativeWindow* window, WaylandBufferPool* pool);
    ~WGacView();

    void StartRendering();
    void StopRendering();
    bool IsRendering() const { return rendering; }

    void Draw();

    cairo_t* GetCairoContext() { return cairoContext; }
    WaylandBuffer* GetCurrentBuffer() { return currentBuffer; }

    int GetWidth() const;
    int GetHeight() const;
};

}
}
}

#endif // WGAC_GAC_VIEW_H
