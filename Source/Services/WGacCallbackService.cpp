#include "WGacCallbackService.h"

namespace vl {
namespace presentation {
namespace wayland {

bool WGacCallbackService::InstallListener(INativeControllerListener* listener)
{
    if (listeners.Contains(listener))
    {
        return false;
    }
    else
    {
        listeners.Add(listener);
        return true;
    }
}

bool WGacCallbackService::UninstallListener(INativeControllerListener* listener)
{
    if (listeners.Contains(listener))
    {
        listeners.Remove(listener);
        return true;
    }
    else
    {
        return false;
    }
}

INativeCallbackInvoker* WGacCallbackService::Invoker()
{
    return this;
}

void WGacCallbackService::InvokeGlobalTimer()
{
    for (vint i = 0; i < listeners.Count(); i++)
    {
        listeners[i]->GlobalTimer();
    }
}

void WGacCallbackService::InvokeClipboardUpdated()
{
    for (vint i = 0; i < listeners.Count(); i++)
    {
        listeners[i]->ClipboardUpdated();
    }
}

void WGacCallbackService::InvokeGlobalShortcutKeyActivated(vint id)
{
    for (vint i = 0; i < listeners.Count(); i++)
    {
        listeners[i]->GlobalShortcutKeyActivated(id);
    }
}

void WGacCallbackService::InvokeNativeWindowCreated(INativeWindow* window)
{
    for (vint i = 0; i < listeners.Count(); i++)
    {
        listeners[i]->NativeWindowCreated(window);
    }
}

void WGacCallbackService::InvokeNativeWindowDestroying(INativeWindow* window)
{
    for (vint i = 0; i < listeners.Count(); i++)
    {
        listeners[i]->NativeWindowDestroying(window);
    }
}

}
}
}
