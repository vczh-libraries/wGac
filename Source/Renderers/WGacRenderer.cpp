#include "WGacRenderer.h"

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

} // namespace wgac
} // namespace elements
} // namespace presentation
} // namespace vl
