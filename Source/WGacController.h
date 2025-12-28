#ifndef WGAC_CONTROLLER_H
#define WGAC_CONTROLLER_H

#include "GacUI.h"

namespace vl {
namespace presentation {
namespace wayland {

extern INativeController* GetWGacController();
extern void DestroyWGacController(INativeController* controller);

}
}
}

#endif // WGAC_CONTROLLER_H
