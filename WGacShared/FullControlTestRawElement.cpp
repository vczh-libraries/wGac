#include "FullControlTestRawElement.h"
#include "GuiWaylandElement.h"
#include <cmath>

namespace demo
{
	using namespace vl;
	using namespace vl::presentation;
	using namespace vl::presentation::elements;
	using namespace vl::presentation::compositions;

	class WaylandClockAttachment : public Object
	{
	protected:
		Ptr<GuiWaylandElement>			element;
		Ptr<IGuiGraphicsEventHandler>	renderingHandler;

		void Render(GuiGraphicsComposition*, GuiWaylandElementEventArgs& arguments)
		{
			if (element->GetId() != L"FullControlTest.Clock") return;
			auto data = element->GetData();
			if (data.Length() != 8 || data[2] != L':' || data[5] != L':') return;
			for (vint i : { 0,1,3,4,6,7 })
			{
				if (data[i] < L'0' || data[i] > L'9') return;
			}
			auto hour = (data[0] - L'0') * 10 + data[1] - L'0';
			auto minute = (data[3] - L'0') * 10 + data[4] - L'0';
			auto second = (data[6] - L'0') * 10 + data[7] - L'0';
			if (hour >= 24 || minute >= 60 || second >= 60) return;

			auto bounds = arguments.bounds;
			double cx = bounds.x1 + bounds.Width() / 2.0;
			double cy = bounds.y1 + bounds.Height() / 2.0;
			double radius = (double)(bounds.Width() < bounds.Height() ? bounds.Width() : bounds.Height()) / 2.0 - 10.0;
			if (radius <= 0) return;
			constexpr double pi = 3.14159265358979323846;
			auto context = arguments.context;
			cairo_set_line_width(context, 2);
			cairo_set_source_rgb(context, 1, 1, 1);
			cairo_arc(context, cx, cy, radius, 0, 2 * pi);
			cairo_stroke(context);
			for (vint i = 0; i < 12; i++)
			{
				auto angle = i * pi / 6;
				cairo_move_to(context, cx + cos(angle) * radius * 0.94, cy + sin(angle) * radius * 0.94);
				cairo_line_to(context, cx + cos(angle) * radius, cy + sin(angle) * radius);
			}
			cairo_stroke(context);

			double angles[] = { (hour + minute / 60.0 + second / 3600.0) * pi / 6 - pi / 2,
				(minute + second / 60.0) * pi / 30 - pi / 2, second * pi / 30 - pi / 2 };
			double lengths[] = { 90.0 / 160, 110.0 / 160, 130.0 / 160 };
			Color colors[] = { Color(0,255,255), Color(0,255,0), Color(255,255,0) };
			for (vint i = 0; i < 3; i++)
			{
				cairo_set_source_rgb(context, colors[i].r / 255.0, colors[i].g / 255.0, colors[i].b / 255.0);
				cairo_move_to(context, cx, cy);
				cairo_line_to(context, cx + cos(angles[i]) * radius * lengths[i], cy + sin(angles[i]) * radius * lengths[i]);
				cairo_stroke(context);
			}
		}
	public:
		WaylandClockAttachment(Ptr<GuiRawElementBase> rawElement)
			: element(rawElement.Cast<GuiWaylandElement>())
		{
			CHECK_ERROR(element, L"FullControlTest clock requires a Wayland raw element.");
			renderingHandler = element->Rendering.AttachMethod(this, &WaylandClockAttachment::Render);
		}

		~WaylandClockAttachment()
		{
			element->Rendering.Detach(renderingHandler);
		}
	};

	Ptr<Object> AttachFullControlTestRawElement(Ptr<GuiRawElementBase> element)
	{
		return Ptr(new WaylandClockAttachment(element));
	}
}
