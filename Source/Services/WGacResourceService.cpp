#include "WGacResourceService.h"
#include <fontconfig/fontconfig.h>

namespace vl {
namespace presentation {
namespace wayland {

WGacResourceService::WGacResourceService()
    : systemCursors(static_cast<vint>(INativeCursor::SystemCursorType::SmallWaiting) + 1)
{
    defaultFont.fontFamily = L"Sans";
    defaultFont.size = 12;
    defaultFont.antialias = true;
}

WGacResourceService::~WGacResourceService()
{
}

INativeCursor* WGacResourceService::GetSystemCursor(INativeCursor::SystemCursorType type)
{
    // Return nullptr for now, cursor support needs separate implementation
    return nullptr;
}

INativeCursor* WGacResourceService::GetDefaultSystemCursor()
{
    return GetSystemCursor(INativeCursor::SystemCursorType::Arrow);
}

FontProperties WGacResourceService::GetDefaultFont()
{
    return defaultFont;
}

void WGacResourceService::SetDefaultFont(const FontProperties& value)
{
    defaultFont = value;
}

void WGacResourceService::EnumerateFonts(collections::List<WString>& fonts)
{
    FcConfig* config = FcInitLoadConfigAndFonts();
    if (!config) return;

    FcPattern* pattern = FcPatternCreate();
    FcObjectSet* os = FcObjectSetBuild(FC_FAMILY, nullptr);
    FcFontSet* fs = FcFontList(config, pattern, os);

    if (fs) {
        for (int i = 0; i < fs->nfont; i++) {
            FcChar8* family = nullptr;
            if (FcPatternGetString(fs->fonts[i], FC_FAMILY, 0, &family) == FcResultMatch) {
                // Convert UTF-8 to WString properly
                const char* utf8Str = reinterpret_cast<const char*>(family);
                fonts.Add(atow(AString(utf8Str)));
            }
        }
        FcFontSetDestroy(fs);
    }

    FcObjectSetDestroy(os);
    FcPatternDestroy(pattern);
    FcConfigDestroy(config);
}

}
}
}
