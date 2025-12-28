#include "WGacRenderer.h"
#include "../WGacWindow.h"
#include "../WGacView.h"
#include <algorithm>
#include <sstream>

namespace vl {
namespace presentation {
namespace elements {
namespace wgac {

namespace {
    IWGacRenderTarget* g_currentRenderTarget = nullptr;
    IWGacObjectProvider* g_objectProvider = nullptr;
    IWGacResourceManager* g_resourceManager = nullptr;
}

void SetCurrentRenderTarget(IWGacRenderTarget* renderTarget) {
    g_currentRenderTarget = renderTarget;
}

IWGacRenderTarget* GetCurrentRenderTarget() {
    return g_currentRenderTarget;
}

cairo_t* GetCurrentCairoContext() {
    if (g_currentRenderTarget) {
        return g_currentRenderTarget->GetCairoContext();
    }
    return nullptr;
}

IWGacObjectProvider* GetWGacObjectProvider() {
    return g_objectProvider;
}

void SetWGacObjectProvider(IWGacObjectProvider* provider) {
    g_objectProvider = provider;
}

IWGacResourceManager* GetWGacResourceManager() {
    return g_resourceManager;
}

void SetWGacResourceManager(IWGacResourceManager* manager) {
    g_resourceManager = manager;
}

// WGacRenderTarget implementation

WGacRenderTarget::WGacRenderTarget(wayland::WGacWindow* window)
    : window(window) {
    view = window->GetView();
}

void WGacRenderTarget::StartRendering() {
    SetCurrentRenderTarget(this);

    cairo_t* cr = GetCairoContext();
    if (cr) {
        cairo_save(cr);
    }
}

RenderTargetFailure WGacRenderTarget::StopRendering() {
    cairo_t* cr = GetCairoContext();
    if (cr) {
        cairo_restore(cr);
    }

    SetCurrentRenderTarget(nullptr);
    clippers.clear();
    clipperCoverWholeTargetCounter = 0;

    return RenderTargetFailure::None;
}

void WGacRenderTarget::PushClipper(Rect clipper) {
    if (clipperCoverWholeTargetCounter > 0) {
        clipperCoverWholeTargetCounter++;
        return;
    }

    Rect previousClipper = GetClipper();
    Rect currentClipper;
    currentClipper.x1 = std::max(previousClipper.x1, clipper.x1);
    currentClipper.y1 = std::max(previousClipper.y1, clipper.y1);
    currentClipper.x2 = std::min(previousClipper.x2, clipper.x2);
    currentClipper.y2 = std::min(previousClipper.y2, clipper.y2);

    if (currentClipper.x1 < currentClipper.x2 && currentClipper.y1 < currentClipper.y2) {
        clippers.push_back(currentClipper);

        cairo_t* cr = GetCairoContext();
        if (cr) {
            cairo_save(cr);
            cairo_rectangle(cr, currentClipper.Left(), currentClipper.Top(),
                           currentClipper.Width(), currentClipper.Height());
            cairo_clip(cr);
        }
    } else {
        clipperCoverWholeTargetCounter++;
    }
}

void WGacRenderTarget::PopClipper() {
    if (clippers.empty()) {
        return;
    }

    if (clipperCoverWholeTargetCounter > 0) {
        clipperCoverWholeTargetCounter--;
    } else {
        clippers.pop_back();
        cairo_t* cr = GetCairoContext();
        if (cr) {
            cairo_restore(cr);
        }
    }
}

Rect WGacRenderTarget::GetClipper() {
    if (clippers.empty()) {
        Size size = GetSize();
        return Rect(0, 0, size.x, size.y);
    }
    return clippers.back();
}

bool WGacRenderTarget::IsClipperCoverWholeTarget() {
    return clipperCoverWholeTargetCounter > 0;
}

cairo_t* WGacRenderTarget::GetCairoContext() {
    if (view) {
        return view->GetCairoContext();
    }
    return nullptr;
}

Size WGacRenderTarget::GetSize() {
    if (view) {
        return Size(view->GetWidth(), view->GetHeight());
    }
    return Size(0, 0);
}

// WGacObjectProvider implementation

void WGacObjectProvider::RecreateRenderTarget(wayland::WGacWindow* window) {
    DestroyRenderTarget(window);
    CreateRenderTarget(window);
}

IWGacRenderTarget* WGacObjectProvider::GetRenderTarget(wayland::WGacWindow* window) {
    auto it = renderTargets.find(window);
    if (it != renderTargets.end()) {
        return it->second.get();
    }
    return nullptr;
}

void WGacObjectProvider::SetRenderTarget(wayland::WGacWindow* window, IWGacRenderTarget* renderTarget) {
    if (renderTarget) {
        // Take ownership - this is a bit awkward but matches the original design
        renderTargets[window] = std::unique_ptr<WGacRenderTarget>(
            static_cast<WGacRenderTarget*>(renderTarget));
    } else {
        renderTargets.erase(window);
    }
}

void WGacObjectProvider::CreateRenderTarget(wayland::WGacWindow* window) {
    if (renderTargets.find(window) == renderTargets.end()) {
        renderTargets[window] = std::make_unique<WGacRenderTarget>(window);
    }
}

void WGacObjectProvider::DestroyRenderTarget(wayland::WGacWindow* window) {
    renderTargets.erase(window);
}

// WGacResourceManager implementation

WGacResourceManager::WGacResourceManager() {
    // Create a small surface for text measurement
    measureSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    measureContext = cairo_create(measureSurface);
    measureLayout = pango_cairo_create_layout(measureContext);
}

WGacResourceManager::~WGacResourceManager() {
    // Clean up font cache
    for (auto& pair : fontCache) {
        pango_font_description_free(pair.second);
    }
    fontCache.clear();

    if (measureLayout) {
        g_object_unref(measureLayout);
    }
    if (measureContext) {
        cairo_destroy(measureContext);
    }
    if (measureSurface) {
        cairo_surface_destroy(measureSurface);
    }
}

std::string WGacResourceManager::GetFontKey(const FontProperties& props) {
    std::ostringstream oss;
    oss << props.fontFamily << "|" << props.size << "|"
        << (props.bold ? "B" : "") << (props.italic ? "I" : "")
        << (props.underline ? "U" : "") << (props.strikeline ? "S" : "");
    return oss.str();
}

PangoFontDescription* WGacResourceManager::CreateFont(const FontProperties& fontProperties) {
    std::string key = GetFontKey(fontProperties);

    auto it = fontCache.find(key);
    if (it != fontCache.end()) {
        return it->second;
    }

    PangoFontDescription* desc = pango_font_description_new();
    pango_font_description_set_family(desc, fontProperties.fontFamily.c_str());
    pango_font_description_set_absolute_size(desc, fontProperties.size * PANGO_SCALE);
    pango_font_description_set_weight(desc, fontProperties.bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
    pango_font_description_set_style(desc, fontProperties.italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);

    fontCache[key] = desc;
    return desc;
}

Size WGacResourceManager::MeasureText(const std::string& text, const FontProperties& font) {
    if (!measureLayout) {
        return Size(0, 0);
    }

    PangoFontDescription* desc = CreateFont(font);
    pango_layout_set_font_description(measureLayout, desc);
    pango_layout_set_text(measureLayout, text.c_str(), -1);

    int width, height;
    pango_layout_get_pixel_size(measureLayout, &width, &height);

    return Size(width, height);
}

} // namespace wgac
} // namespace elements
} // namespace presentation
} // namespace vl
