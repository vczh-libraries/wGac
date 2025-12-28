#ifndef WGAC_RENDERER_H
#define WGAC_RENDERER_H

#include "GacUI.h"
#include <cairo/cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

namespace vl {
namespace presentation {
namespace wayland {

class WGacNativeWindow;
class WGacView;

}

namespace elements {
namespace wgac {

class IWGacRenderTarget : public Object, public IGuiGraphicsRenderTarget
{
public:
    virtual cairo_t* GetCairoContext() = 0;
};

class IWGacObjectProvider : public Interface
{
public:
    virtual void RecreateRenderTarget(INativeWindow* window) = 0;
    virtual IWGacRenderTarget* GetWGacRenderTarget(INativeWindow* window) = 0;
    virtual IWGacRenderTarget* GetBindedRenderTarget(INativeWindow* window) = 0;
    virtual void SetBindedRenderTarget(INativeWindow* window, IWGacRenderTarget* renderTarget) = 0;
};

class IWGacResourceManager : public Interface
{
public:
    virtual PangoFontDescription* CreateWGacFont(const FontProperties& fontProperties) = 0;
};

extern void SetCurrentRenderTarget(IWGacRenderTarget* renderTarget);
extern IWGacRenderTarget* GetCurrentRenderTarget();
extern IWGacObjectProvider* GetWGacObjectProvider();
extern void SetWGacObjectProvider(IWGacObjectProvider* provider);
extern IWGacResourceManager* GetWGacResourceManager();
extern void SetWGacResourceManager(IWGacResourceManager* manager);

inline cairo_t* GetCurrentWGacContextFromRenderTarget()
{
    auto* target = GetCurrentRenderTarget();
    return target ? target->GetCairoContext() : nullptr;
}

extern int SetupWGacRenderer();

}
}
}
}

#endif // WGAC_RENDERER_H
