#ifndef VCZH_WGAC_FULLCONTROLTESTRAWELEMENTREMOTE
#define VCZH_WGAC_FULLCONTROLTESTRAWELEMENTREMOTE

#include "GacUI.h"

namespace demo
{
	// Tracks manual Touching painters only. Clock is selected by its named interpretor.
	// Destroy this attachment before the renderer. Disconnection retains the last picture.
	extern vl::Ptr<vl::Object> AttachFullControlTestRawElements(vl::presentation::remote_renderer::GuiRemoteRendererSingle& renderer);
}

#endif
