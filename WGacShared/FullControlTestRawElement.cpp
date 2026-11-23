#include "FullControlTestRawElement.h"
#include "GuiWaylandElement.h"
#include <cmath>

namespace demo
{
	using namespace vl;
	using namespace vl::presentation;
	using namespace vl::presentation::elements;
	using namespace vl::presentation::compositions;

	class WaylandTouchingAttachment : public Object
	{
	protected:
		Ptr<GuiWaylandElement>			element;
		Ptr<IGuiGraphicsEventHandler>	renderingHandler;

		void Render(GuiGraphicsComposition*, GuiWaylandElementEventArgs& arguments)
		{
			auto data = element->GetData();
			collections::List<double> values;
			vint start = 0;
			for (vint i = 0; i <= data.Length(); i++)
			{
				if (i != data.Length() && data[i] != L',') continue;
				bool success = false;
				auto value = wtof_test(data.Sub(start, i - start), success);
				if (!success || !std::isfinite(value)) return;
				values.Add(value);
				start = i + 1;
			}
			if (values.Count() < 5 || (values.Count() - 5) % 3 != 0 || values[4] != (values.Count() - 5) / 3) return;

			auto context = arguments.context;
			auto bounds = arguments.bounds;
			double x[] = { -80,80,80,-80 };
			double y[] = { -50,-50,50,50 };
			cairo_new_path(context);
			for (vint i = 0; i < 4; i++)
			{
				auto px = bounds.x1 + values[0] + x[i] * values[2] - y[i] * values[3];
				auto py = bounds.y1 + values[1] + x[i] * values[3] + y[i] * values[2];
				if (i == 0) cairo_move_to(context, px, py);
				else cairo_line_to(context, px, py);
			}
			cairo_close_path(context);
			cairo_set_source_rgb(context, 135.0 / 255, 206.0 / 255, 235.0 / 255);
			cairo_fill_preserve(context);
			cairo_set_line_width(context, 2);
			cairo_set_source_rgb(context, 0, 0, 0);
			cairo_stroke(context);
			cairo_set_source_rgb(context, 1, 136.0 / 255, 0);
			for (vint i = 5; i < values.Count(); i += 3)
			{
				cairo_arc(context, bounds.x1 + (vint)values[i + 1], bounds.y1 + (vint)values[i + 2], 16, 0, 6.28318530717958647692);
				cairo_stroke(context);
			}
		}

	public:
		WaylandTouchingAttachment(Ptr<GuiRawElementBase> rawElement)
			: element(rawElement.Cast<GuiWaylandElement>())
		{
			CHECK_ERROR(element, L"FullControlTest Touching requires a Wayland raw element.");
			renderingHandler = element->Rendering.AttachMethod(this, &WaylandTouchingAttachment::Render);
		}

		~WaylandTouchingAttachment()
		{
			element->Rendering.Detach(renderingHandler);
		}
	};

	Ptr<Object> AttachFullControlTestTouchingElement(Ptr<GuiRawElementBase> element)
	{
		return Ptr(new WaylandTouchingAttachment(element));
	}
}
