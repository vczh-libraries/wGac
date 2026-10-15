#include "GuiWaylandElement.h"

namespace vl::presentation::elements
{
	vint GuiWaylandElement::GetElementType()
	{
		return GetElementTypeInternal<GuiWaylandElement>();
	}

	GuiWaylandElement* GuiWaylandElement::Create()
	{
		return CreateInternal<GuiWaylandElement>();
	}

	namespace wgac
	{
		void GuiWaylandElementRenderer::InitializeInternal()
		{
		}

		void GuiWaylandElementRenderer::FinalizeInternal()
		{
		}

		void GuiWaylandElementRenderer::RenderTargetChangedInternal(IWGacRenderTarget*, IWGacRenderTarget*)
		{
		}

		void GuiWaylandElementRenderer::Render(Rect bounds)
		{
			if (!renderTarget) return;
			auto context = renderTarget->GetCairoContext();
			if (!context) return;

			// Clipping consumes the current path, and cairo_save/restore do not preserve it.
			auto path = cairo_copy_path(context);
			cairo_new_path(context);
			renderTarget->PushClipper(bounds, element);
			if (!renderTarget->IsClipperCoverWholeTarget())
			{
				cairo_save(context);
				cairo_new_path(context);
				GuiWaylandElementEventArgs arguments(element, context, bounds);
				element->Rendering.Execute(arguments);
				cairo_restore(context);
			}
			renderTarget->PopClipper(element);
			cairo_new_path(context);
			cairo_append_path(context, path);
			cairo_path_destroy(path);
		}

		void GuiWaylandElementRenderer::OnElementStateChanged()
		{
		}
	}
}
