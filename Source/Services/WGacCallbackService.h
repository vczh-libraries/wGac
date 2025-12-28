#ifndef WGAC_CALLBACKSERVICE_H
#define WGAC_CALLBACKSERVICE_H

#include "GacUI.h"

namespace vl {
namespace presentation {
namespace wayland {

class WGacCallbackService : public Object, public INativeCallbackService, public INativeCallbackInvoker
{
protected:
    collections::List<INativeControllerListener*> listeners;

public:
    bool InstallListener(INativeControllerListener* listener) override;
    bool UninstallListener(INativeControllerListener* listener) override;
    INativeCallbackInvoker* Invoker() override;

    void InvokeGlobalTimer() override;
    void InvokeClipboardUpdated() override;
    void InvokeGlobalShortcutKeyActivated(vint id) override;
    void InvokeNativeWindowCreated(INativeWindow* window) override;
    void InvokeNativeWindowDestroying(INativeWindow* window) override;
};

}
}
}

#endif // WGAC_CALLBACKSERVICE_H
