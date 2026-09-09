#ifndef WGAC_TUI_CLIPBOARDSERVICE_H
#define WGAC_TUI_CLIPBOARDSERVICE_H

#include "GacUI.h"

namespace vl::presentation::wayland
{
	class TuiWGacClipboardService : public Object, public INativeClipboardService
	{
		class Impl;
		Ptr<Impl>					impl;

	public:
		TuiWGacClipboardService();
		~TuiWGacClipboardService();
		bool						PumpEvents();
		Ptr<INativeClipboardReader>	ReadClipboard() override;
		Ptr<INativeClipboardWriter>	WriteClipboard() override;
	};
}

#endif
