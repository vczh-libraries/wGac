#ifndef WGAC_INPUTSERVICE_H
#define WGAC_INPUTSERVICE_H

#include "GacUI.h"
#include <memory>

namespace vl {
namespace presentation {
namespace wayland {

class WGacGlobalShortcutService;

class WGacInputService : public Object, public INativeInputService
{
    typedef void (*TimerFunc)();

protected:
    TimerFunc timer;
    bool isTimerEnabled;
    vint usedHotKeys;
    collections::Dictionary<WString, VKEY> keys;
    collections::Array<WString> keyNames;
    std::unique_ptr<WGacGlobalShortcutService> globalShortcuts;

public:
    WGacInputService(TimerFunc timer, const Func<void(vint)>& shortcutActivated = {});
    ~WGacInputService();

    void PumpEvents();

    void StartTimer() override;
    void StopTimer() override;
    bool IsTimerEnabled() override;
    bool IsKeyPressing(VKEY code) override;
    bool IsKeyToggled(VKEY code) override;
    WString GetKeyName(VKEY code) override;
    VKEY GetKey(const WString& name) override;
    vint RegisterGlobalShortcutKey(bool ctrl, bool shift, bool alt, bool osSuper, VKEY code) override;
    bool UnregisterGlobalShortcutKey(vint id) override;

    void InitKeyMapping();
    WString GetKeyNameInternal(VKEY code);
};

}
}
}

#endif // WGAC_INPUTSERVICE_H
