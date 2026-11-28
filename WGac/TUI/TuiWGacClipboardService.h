#ifndef WGAC_TUI_CLIPBOARDSERVICE_H
#define WGAC_TUI_CLIPBOARDSERVICE_H

#include "GacUI.h"

namespace vl::presentation::wayland
{
	class TuiWGacClipboardService : public Object, public INativeClipboardService, public IGuiRemoteClipboardEventSource
	{
		class Impl;
		Ptr<Impl>					impl;

	public:
		explicit TuiWGacClipboardService(bool allowPrivateClipboard = true);
		~TuiWGacClipboardService();
		bool						PumpClipboardEvents() override;
		Ptr<INativeClipboardReader>	ReadClipboard() override;
		Ptr<INativeClipboardWriter>	WriteClipboard() override;
	};
}

#endif
