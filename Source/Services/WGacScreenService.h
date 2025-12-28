#ifndef WGAC_SCREENSERVICE_H
#define WGAC_SCREENSERVICE_H

#include "GacUI.h"

namespace vl {
namespace presentation {
namespace wayland {

class WGacScreen : public Object, public INativeScreen
{
protected:
    NativeRect bounds;
    WString name;
    bool primary;
    double scalingX;
    double scalingY;

public:
    WGacScreen(const NativeRect& bounds, const WString& name, bool primary, double scale = 1.0);

    double GetScalingX() override;
    double GetScalingY() override;
    NativeRect GetBounds() override;
    NativeRect GetClientBounds() override;
    WString GetName() override;
    bool IsPrimary() override;
};

class WGacScreenService : public Object, public INativeScreenService
{
protected:
    collections::List<Ptr<WGacScreen>> monitors;

public:
    void RefreshScreenInformation();
    vint GetScreenCount() override;
    INativeScreen* GetScreen(vint index) override;
    INativeScreen* GetScreen(INativeWindow* window) override;
};

}
}
}

#endif // WGAC_SCREENSERVICE_H
