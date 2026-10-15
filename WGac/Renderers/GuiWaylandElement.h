#ifndef VCZH_PRESENTATION_ELEMENTS_GUIWAYLANDELEMENT
#define VCZH_PRESENTATION_ELEMENTS_GUIWAYLANDELEMENT

#include "WGacRenderer.h"

namespace vl::presentation::elements
{
	class GuiWaylandElement;

	struct GuiWaylandElementEventArgs : compositions::GuiEventArgs
	{
		GuiWaylandElement*		element;
		// Borrowed for this callback only. Do not retain/destroy the context or its surface.
		cairo_t*				context;
		Rect					bounds;

		GuiWaylandElementEventArgs(GuiWaylandElement* _element, cairo_t* _context, Rect _bounds)
			: element(_element), context(_context), bounds(_bounds)
		{
		}
	};

	class GuiWaylandElement : public GuiRawElementBase
	{
		friend class GuiElementBase<GuiRawElementBase>;
		static constexpr const wchar_t* ElementTypeName = L"WaylandElement";
	protected:
		GuiWaylandElement() = default;
	public:
		static vint					GetElementType();
		static GuiWaylandElement*	Create();

		// Bounds and context use the current logical coordinates, clip and DPI transform.
		compositions::GuiGraphicsEvent<GuiWaylandElementEventArgs>	Rendering;
	};

	namespace wgac
	{
		class GuiWaylandElementRenderer : public GuiElementRendererBase<GuiWaylandElement, GuiWaylandElementRenderer, IWGacRenderTarget>
		{
			friend class GuiElementRendererBase<GuiWaylandElement, GuiWaylandElementRenderer, IWGacRenderTarget>;
		protected:
			void			InitializeInternal();
			void			FinalizeInternal();
			void			RenderTargetChangedInternal(IWGacRenderTarget* oldTarget, IWGacRenderTarget* newTarget);
		public:
			void			Render(Rect bounds) override;
			void			OnElementStateChanged() override;
		};
	}
}

#endif
