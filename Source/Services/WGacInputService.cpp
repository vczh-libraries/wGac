#include "WGacInputService.h"
#include "WGacCallbackService.h"
#include <xkbcommon/xkbcommon.h>
#include <poll.h>

namespace vl {
namespace presentation {
namespace wayland {

WGacInputService::WGacInputService(TimerFunc _timer)
    : usedHotKeys(0)
    , keyNames(256)
    , timer(_timer)
    , isTimerEnabled(false)
{
    InitKeyMapping();
}

WString WGacInputService::GetKeyNameInternal(VKEY code)
{
    if ((vint)code < 8) return L"?";

    // Map common keys
    switch ((vint)code) {
        case 0x08: return L"Backspace";
        case 0x09: return L"Tab";
        case 0x0D: return L"Enter";
        case 0x10: return L"Shift";
        case 0x11: return L"Ctrl";
        case 0x12: return L"Alt";
        case 0x14: return L"CapsLock";
        case 0x1B: return L"Escape";
        case 0x20: return L"Space";
        case 0x21: return L"PageUp";
        case 0x22: return L"PageDown";
        case 0x23: return L"End";
        case 0x24: return L"Home";
        case 0x25: return L"Left";
        case 0x26: return L"Up";
        case 0x27: return L"Right";
        case 0x28: return L"Down";
        case 0x2D: return L"Insert";
        case 0x2E: return L"Delete";
        default: break;
    }

    // Letters A-Z
    if (code >= VKEY::KEY_A && code <= VKEY::KEY_Z) {
        wchar_t buf[2] = { static_cast<wchar_t>(L'A' + ((vint)code - (vint)VKEY::KEY_A)), 0 };
        return WString(buf);
    }

    // Numbers 0-9
    if (code >= VKEY::KEY_0 && code <= VKEY::KEY_9) {
        wchar_t buf[2] = { static_cast<wchar_t>(L'0' + ((vint)code - (vint)VKEY::KEY_0)), 0 };
        return WString(buf);
    }

    // Function keys F1-F12
    if (code >= VKEY::KEY_F1 && code <= VKEY::KEY_F12) {
        return L"F" + itow((vint)code - (vint)VKEY::KEY_F1 + 1);
    }

    return L"?";
}

void WGacInputService::InitKeyMapping()
{
    for (vint i = 0; i < keyNames.Count(); i++)
    {
        keyNames[i] = GetKeyNameInternal((VKEY)i);
        if (keyNames[i] != L"?")
        {
            keys.Set(keyNames[i], (VKEY)i);
        }
    }
}

void WGacInputService::StartTimer()
{
    isTimerEnabled = true;
}

void WGacInputService::StopTimer()
{
    isTimerEnabled = false;
}

bool WGacInputService::IsTimerEnabled()
{
    return isTimerEnabled;
}

bool WGacInputService::IsKeyPressing(VKEY code)
{
    return false;
}

bool WGacInputService::IsKeyToggled(VKEY code)
{
    return false;
}

WString WGacInputService::GetKeyName(VKEY code)
{
    if (0 <= (vint)code && (vint)code < keyNames.Count())
    {
        return keyNames[(vint)code];
    }
    else
    {
        return L"?";
    }
}

VKEY WGacInputService::GetKey(const WString& name)
{
    vint index = keys.Keys().IndexOf(name);
    return index == -1 ? VKEY::KEY_UNKNOWN : keys.Values()[index];
}

vint WGacInputService::RegisterGlobalShortcutKey(bool ctrl, bool shift, bool alt, VKEY code)
{
    // Global shortcuts not fully implemented yet
    ++usedHotKeys;
    return usedHotKeys;
}

bool WGacInputService::UnregisterGlobalShortcutKey(vint id)
{
    if (id <= usedHotKeys) {
        return true;
    }
    return false;
}

}
}
}
