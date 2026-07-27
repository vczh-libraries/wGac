#ifndef WGAC_CONTROLLER_H
#define WGAC_CONTROLLER_H

#include "GacUI.h"

namespace vl {
namespace presentation {
namespace wayland {

class WGacNativeWindow;

extern INativeController* GetWGacController();
extern void DestroyWGacController(INativeController* controller);
extern void GetAllCreatedWGacNativeWindows(collections::List<WGacNativeWindow*>& windows);

}
}
}

#endif // WGAC_CONTROLLER_H
