#ifndef WGAC_RESOURCESERVICE_H
#define WGAC_RESOURCESERVICE_H

#include "GacUI.h"
#include "../WGacCursor.h"

namespace vl {
namespace presentation {
namespace wayland {

class WGacResourceService : public Object, public INativeResourceService
{
protected:
    collections::Array<Ptr<WGacSystemCursor>> systemCursors;
    FontProperties defaultFont;

public:
    WGacResourceService();
    virtual ~WGacResourceService();

    INativeCursor* GetSystemCursor(INativeCursor::SystemCursorType type) override;
    INativeCursor* GetDefaultSystemCursor() override;
    FontProperties GetDefaultFont() override;
    void SetDefaultFont(const FontProperties& value) override;
    void EnumerateFonts(collections::List<WString>& fonts) override;

    INativeCursor* ResolveSystemCursor(INativeCursor* cursor);
};

}
}
}

#endif // WGAC_RESOURCESERVICE_H
