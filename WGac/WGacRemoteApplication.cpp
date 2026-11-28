#include "TUI/TuiWGacClipboardService.h"

int SetupRemoteNativeController(vl::presentation::IGuiRemoteProtocol* protocol, const vl::presentation::GuiRemoteControllerConfig& config)
{
	vl::presentation::wayland::TuiWGacClipboardService clipboard(false);
	return SetupRemoteNativeControllerInternal(protocol, config, &clipboard);
}
