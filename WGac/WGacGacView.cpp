#include "WGacGacView.h"
#include "WGacNativeWindow.h"

namespace vl {
namespace presentation {
namespace wayland {

WGacView::WGacView(WGacNativeWindow* _window, WaylandBufferPool* pool)
    : window(_window)
    , bufferPool(pool)
    , currentBuffer(nullptr)
    , cairoContext(nullptr)
    , rendering(false)
{
}

WGacView::~WGacView()
{
    if (cairoContext) {
        cairo_destroy(cairoContext);
        cairoContext = nullptr;
    }
}

void WGacView::StartRendering()
{
    if (rendering) return;

    currentBuffer = bufferPool->GetNextBuffer();
    if (currentBuffer) {
        currentBuffer->BeginDraw();
        auto* surface = currentBuffer->GetCairoSurface();
        if (surface) {
            cairoContext = cairo_create(surface);
        }
        rendering = true;
    }
}

void WGacView::StopRendering()
{
    if (!rendering) return;

    if (cairoContext) {
        cairo_destroy(cairoContext);
        cairoContext = nullptr;
    }

    // Flush the buffer after rendering
    if (currentBuffer) {
        currentBuffer->EndDraw();
    }

    rendering = false;
}

void WGacView::Draw()
{
    StartRendering();

    if (!cairoContext) return;

    // Clear background with dark color (DarkSkin default)
    cairo_set_source_rgb(cairoContext, 0.15, 0.15, 0.15);
    cairo_paint(cairoContext);

    StopRendering();
}

int WGacView::GetWidth() const
{
    return bufferPool ? bufferPool->GetWidth() : 0;
}

int WGacView::GetHeight() const
{
    return bufferPool ? bufferPool->GetHeight() : 0;
}

}
}
}
