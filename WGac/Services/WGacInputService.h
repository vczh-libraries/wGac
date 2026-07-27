#ifndef WGAC_INPUTSERVICE_H
#define WGAC_INPUTSERVICE_H

#include "GacUI.h"

namespace vl {
namespace presentation {
namespace wayland {

class WGacInputService : public Object, public INativeInputService
{
    typedef void (*TimerFunc)();

protected:
    TimerFunc timer;
    bool isTimerEnabled;
    int usedHotKeys;
    collections::Dictionary<WString, VKEY> keys;
    collections::Array<WString> keyNames;

public:
    WGacInputService(TimerFunc timer);

    void StartTimer() override;
    void StopTimer() override;
    bool IsTimerEnabled() override;
    bool IsKeyPressing(VKEY code) override;
    bool IsKeyToggled(VKEY code) override;
    WString GetKeyName(VKEY code) override;
    VKEY GetKey(const WString& name) override;
    vint RegisterGlobalShortcutKey(bool ctrl, bool shift, bool alt, VKEY code) override;
    bool UnregisterGlobalShortcutKey(vint id) override;

    void InitKeyMapping();
    WString GetKeyNameInternal(VKEY code);
};

}
}
}

#endif // WGAC_INPUTSERVICE_H
