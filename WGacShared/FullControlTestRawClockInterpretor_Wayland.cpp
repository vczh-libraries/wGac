#include "FullControlTestRawClockInterpretor.h"
#include "GuiWaylandElement.h"

namespace demo
{
	using namespace vl;
	using namespace vl::presentation;
	using namespace vl::presentation::elements;
	using namespace vl::presentation::compositions;

	class RawClockWaylandInterpretor : public RawClockInterpretorBase
	{
	protected:
		Ptr<IGuiGraphicsEventHandler>	renderingHandler;
		cairo_t*						context = nullptr;

		void SelectStroke(Stroke stroke) override
		{
			auto color = GetStrokeColor(stroke);
			cairo_set_source_rgb(context, color.r / 255.0, color.g / 255.0, color.b / 255.0);
			cairo_set_line_width(context, GetStrokeWidth(stroke));
		}

		void DrawLine(double x1, double y1, double x2, double y2) override
		{
			cairo_move_to(context, x1, y1);
			cairo_line_to(context, x2, y2);
			cairo_stroke(context);
		}

		void DrawEllipse(double cx, double cy, double radius) override
		{
			cairo_arc(context, cx, cy, radius, 0, 6.28318530717958647692);
			cairo_stroke(context);
		}

		void Render(GuiGraphicsComposition*, GuiWaylandElementEventArgs& arguments)
		{
			// Borrow the provider's already transformed and clipped context for this paint only.
			context = arguments.context;
			DrawClock(arguments.bounds);
			context = nullptr;
		}

	public:
		void Initialize(IGuiGraphicsElement* _element) override
		{
			auto wayland = dynamic_cast<GuiWaylandElement*>(_element);
			element = wayland;
			renderingHandler = wayland->Rendering.AttachMethod(this, &RawClockWaylandInterpretor::Render);
		}

		void Finalize() override
		{
			auto wayland = static_cast<GuiWaylandElement*>(element);
			wayland->Rendering.Detach(renderingHandler);
			renderingHandler = nullptr;
		}
	};

	class RawClockWaylandInterpretorFactory : public Object, public IGuiGraphicsRawInterpretorFactory
	{
	public:
		IGuiGraphicsRawInterpretor* Create() override
		{
			return new RawClockWaylandInterpretor;
		}
	};

	class FullControlTestClockInterpretorPlugin : public Object, public IGuiPlugin
	{
	public:
		GUI_PLUGIN_NAME(FullControlTest_ClockInterpretor)
		{
		}

		void Load(bool controllerUnrelatedPlugins, bool controllerRelatedPlugins) override
		{
			if (controllerRelatedPlugins)
			{
				GetGuiGraphicsResourceManager()->RegisterRawInterpretorFactory(L"FullControlTest::Clock", Ptr(new RawClockWaylandInterpretorFactory));
			}
		}

		void Unload(bool controllerUnrelatedPlugins, bool controllerRelatedPlugins) override
		{
		}
	};
	GUI_REGISTER_PLUGIN(FullControlTestClockInterpretorPlugin)
}
