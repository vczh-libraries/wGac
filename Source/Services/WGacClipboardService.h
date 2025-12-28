#ifndef WGAC_CLIPBOARDSERVICE_H
#define WGAC_CLIPBOARDSERVICE_H

#include "GacUI.h"

namespace vl {
namespace presentation {
namespace wayland {

class WGacClipboardService : public Object, public INativeClipboardService
{
public:
    WGacClipboardService();
    virtual ~WGacClipboardService();

    Ptr<INativeClipboardReader> ReadClipboard() override;
    Ptr<INativeClipboardWriter> WriteClipboard() override;
};

}
}
}

#endif // WGAC_CLIPBOARDSERVICE_H
