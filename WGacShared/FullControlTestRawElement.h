#ifndef VCZH_WGAC_FULLCONTROLTESTRAWELEMENT
#define VCZH_WGAC_FULLCONTROLTESTRAWELEMENT

#include "GacUI.h"

namespace demo
{
	// Retain through the window's drawing lifetime; destruction detaches the callback.
	// Data: centerX,centerY,axisX,axisY,count followed by count triples of pointId,x,y.
	// The axis rotates and uniformly scales a 160 by 100 rectangle.
	extern vl::Ptr<vl::Object> AttachFullControlTestTouchingElement(vl::Ptr<vl::presentation::elements::GuiRawElementBase> element);
}

#endif
