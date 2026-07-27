#include "WGacScreenService.h"
#include "../Wayland/WaylandDisplay.h"

namespace vl {
namespace presentation {
namespace wayland {

WGacScreen::WGacScreen(const NativeRect& _bounds, const WString& _name, bool _primary, double scale)
    : bounds(_bounds)
    , name(_name)
    , primary(_primary)
    , scalingX(scale)
    , scalingY(scale)
{
}

double WGacScreen::GetScalingX()
{
    return scalingX;
}

double WGacScreen::GetScalingY()
{
    return scalingY;
}

NativeRect WGacScreen::GetBounds()
{
    return bounds;
}

NativeRect WGacScreen::GetClientBounds()
{
    return bounds;
}

WString WGacScreen::GetName()
{
    return name;
}

bool WGacScreen::IsPrimary()
{
    return primary;
}

void WGacScreenService::RefreshScreenInformation()
{
    monitors.Clear();
    // Get scale factor from Wayland display
    double scale = 1.0;
    WaylandDisplay* display = GetWaylandDisplay();
    if (display) {
        scale = display->GetOutputScale();
    }
    // Add a default screen for now
    // In a full implementation, this would query Wayland outputs
    monitors.Add(Ptr(new WGacScreen(
        NativeRect(0, 0, 1920, 1080),
        L"Default Monitor",
        true,
        scale
    )));
}

vint WGacScreenService::GetScreenCount()
{
    return monitors.Count();
}

INativeScreen* WGacScreenService::GetScreen(vint index)
{
    if (index >= 0 && index < monitors.Count())
    {
        return monitors[index].Obj();
    }
    return nullptr;
}

INativeScreen* WGacScreenService::GetScreen(INativeWindow* window)
{
    // Return primary screen for now
    if (monitors.Count() > 0)
    {
        return monitors[0].Obj();
    }
    return nullptr;
}

}
}
}
