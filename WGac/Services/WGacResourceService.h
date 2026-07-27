#ifndef WGAC_RESOURCESERVICE_H
#define WGAC_RESOURCESERVICE_H

#include "GacUI.h"

namespace vl {
namespace presentation {
namespace wayland {

class WGacCursor;

class WGacResourceService : public Object, public INativeResourceService
{
protected:
    collections::Array<Ptr<INativeCursor>> systemCursors;
    FontProperties defaultFont;

public:
    WGacResourceService();
    virtual ~WGacResourceService();

    INativeCursor* GetSystemCursor(INativeCursor::SystemCursorType type) override;
    INativeCursor* GetDefaultSystemCursor() override;
    FontProperties GetDefaultFont() override;
    void SetDefaultFont(const FontProperties& value) override;
    void EnumerateFonts(collections::List<WString>& fonts) override;
};

}
}
}

#endif // WGAC_RESOURCESERVICE_H
