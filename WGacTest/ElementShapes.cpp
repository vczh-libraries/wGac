#include "Renderers/WGacRenderer.h"

using namespace vl;
using namespace vl::presentation;
using namespace vl::presentation::elements;
using namespace vl::presentation::elements::wgac;

class ShapeRenderTarget : public IWGacRenderTarget
{
public:
	cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 64, 64);
	cairo_t* context = cairo_create(surface);

	~ShapeRenderTarget()
	{
		cairo_destroy(context);
		cairo_surface_destroy(surface);
	}

	cairo_t* GetCairoContext() override { return context; }
	Size GetCanvasSize() override { return { 64,64 }; }
	void StartRenderingOnNativeWindow() override { SetCurrentRenderTarget(this); }
	RenderTargetFailure StopRenderingOnNativeWindow() override
	{
		TEST_ASSERT(cairo_status(context) == CAIRO_STATUS_SUCCESS);
		SetCurrentRenderTarget(nullptr);
		return RenderTargetFailure::None;
	}
	void AfterPushedClipper(Rect, Rect area, reflection::DescriptableObject*) override
	{
		cairo_save(context);
		cairo_rectangle(context, area.x1, area.y1, area.Width(), area.Height());
		cairo_clip(context);
	}
	void AfterPushedClipperAndBecameInvalid(Rect, reflection::DescriptableObject*) override {}
	void AfterPoppedClipperAndBecameValid(Rect, bool, reflection::DescriptableObject*) override {}
	void AfterPoppedClipper(Rect, bool, reflection::DescriptableObject*) override { cairo_restore(context); }

	vuint32_t Pixel(vint x, vint y)
	{
		cairo_surface_flush(surface);
		auto pixels = reinterpret_cast<vuint32_t*>(cairo_image_surface_get_data(surface));
		return pixels[y * cairo_image_surface_get_stride(surface) / 4 + x];
	}

	void Draw(IGuiGraphicsElement* element, Rect bounds = { 8,8,56,56 })
	{
		auto renderer = element->GetRenderer();
		renderer->SetRenderTarget(this);
		StartRendering();
		renderer->Render(bounds);
		StopRendering();
		renderer->SetRenderTarget(nullptr);
	}
};

TEST_FILE
{
	TEST_CASE(L"Gradient backgrounds respect every shape and gradient direction")
	{
		for (auto shape : { ElementShapeType::Rectangle, ElementShapeType::Ellipse, ElementShapeType::RoundRect })
		{
			for (auto direction : { GuiGradientBackgroundElement::Horizontal, GuiGradientBackgroundElement::Vertical, GuiGradientBackgroundElement::Slash, GuiGradientBackgroundElement::Backslash })
			{
				ShapeRenderTarget target;
				auto element = Ptr(GuiGradientBackgroundElement::Create());
				element->SetShape({ shape,12,6 });
				element->SetColors(Color(255, 0, 0), Color(0, 0, 255));
				element->SetDirection(direction);
				target.Draw(element.Obj());
				TEST_ASSERT(target.Pixel(32, 32) >> 24 == 255);
				TEST_ASSERT(target.Pixel(8, 8) == 0 || shape == ElementShapeType::Rectangle);
				TEST_ASSERT(target.Pixel(7, 32) == 0);
				TEST_ASSERT(target.Pixel(56, 32) == 0);
				Point first, last;
				switch (direction)
				{
				case GuiGradientBackgroundElement::Horizontal: first = { 16,32 }; last = { 48,32 }; break;
				case GuiGradientBackgroundElement::Vertical: first = { 32,16 }; last = { 32,48 }; break;
				case GuiGradientBackgroundElement::Slash: first = { 44,20 }; last = { 20,44 }; break;
				case GuiGradientBackgroundElement::Backslash: first = { 20,20 }; last = { 44,44 }; break;
				}
				auto a = target.Pixel(first.x, first.y);
				auto b = target.Pixel(last.x, last.y);
				TEST_ASSERT(((a >> 16) & 255) > (a & 255));
				TEST_ASSERT(((b >> 16) & 255) < (b & 255));
			}
		}
	});

	TEST_CASE(L"Solid and constant gradient backgrounds have identical silhouettes")
	{
		for (auto shape : { ElementShape{ ElementShapeType::Rectangle }, ElementShape{ ElementShapeType::Ellipse }, ElementShape{ ElementShapeType::RoundRect,12,4 } })
		{
			ShapeRenderTarget solidTarget, gradientTarget;
			auto solid = Ptr(GuiSolidBackgroundElement::Create());
			solid->SetShape(shape);
			solid->SetColor(Color(30, 90, 180, 128));
			auto gradient = Ptr(GuiGradientBackgroundElement::Create());
			gradient->SetShape(shape);
			gradient->SetColors(solid->GetColor(), solid->GetColor());
			solidTarget.Draw(solid.Obj(), { 8,8,56,40 });
			gradientTarget.Draw(gradient.Obj(), { 8,8,56,40 });
			TEST_ASSERT(solidTarget.Pixel(32, 24) >> 24 == 128);
			for (vint y = 0; y < 64; y++)
			{
				for (vint x = 0; x < 64; x++) TEST_ASSERT(solidTarget.Pixel(x, y) == gradientTarget.Pixel(x, y));
			}
		}
	});

	TEST_CASE(L"Rounded fills and borders use the vertical radius independently")
	{
		ShapeRenderTarget fillTarget, borderTarget;
		auto fill = Ptr(GuiSolidBackgroundElement::Create());
		fill->SetColor(Color(255, 255, 255));
		fill->SetShape({ ElementShapeType::RoundRect,12,3 });
		fillTarget.Draw(fill.Obj());
		TEST_ASSERT(fillTarget.Pixel(8, 14) == 0xFFFFFFFF);
		TEST_ASSERT(fillTarget.Pixel(8, 8) == 0);
		auto border = Ptr(GuiSolidBorderElement::Create());
		border->SetColor(Color(255, 255, 255));
		border->SetShape(fill->GetShape());
		borderTarget.Draw(border.Obj());
		TEST_ASSERT(borderTarget.Pixel(8, 14) == 0xFFFFFFFF);
		TEST_ASSERT(borderTarget.Pixel(32, 32) == 0);
		TEST_ASSERT(borderTarget.Pixel(7, 32) == 0);
		TEST_ASSERT(borderTarget.Pixel(56, 32) == 0);
	});

	TEST_CASE(L"Rounded shapes clamp oversized radii and allow square corners")
	{
		for (auto element : { Ptr(GuiSolidBackgroundElement::Create()).Cast<IGuiGraphicsElement>(), Ptr(GuiSolidBorderElement::Create()).Cast<IGuiGraphicsElement>(), Ptr(GuiGradientBackgroundElement::Create()).Cast<IGuiGraphicsElement>() })
		{
			for (auto radius : { Size(100, 100), Size(0, 8), Size(8, 0) })
			{
				ShapeRenderTarget target;
				auto shape = ElementShape{ ElementShapeType::RoundRect,radius.x,radius.y };
				if (auto solid = element.Cast<GuiSolidBackgroundElement>()) { solid->SetShape(shape); solid->SetColor(Color(255, 255, 255)); }
				if (auto border = element.Cast<GuiSolidBorderElement>()) { border->SetShape(shape); border->SetColor(Color(255, 255, 255)); }
				if (auto gradient = element.Cast<GuiGradientBackgroundElement>()) { gradient->SetShape(shape); gradient->SetColors(Color(255, 255, 255), Color(255, 255, 255)); }
				target.Draw(element.Obj(), { 8,8,56,24 });
				TEST_ASSERT(target.Pixel(32, 8) >> 24 > 0);
				TEST_ASSERT(target.Pixel(7, 16) == 0);
				TEST_ASSERT(target.Pixel(56, 16) == 0);
				TEST_ASSERT(target.Pixel(32, 7) == 0);
				TEST_ASSERT(target.Pixel(32, 24) == 0);
				TEST_ASSERT((target.Pixel(8, 8) == 0) == (radius.x > 0 && radius.y > 0));
			}
		}
	});

	TEST_CASE(L"Empty shapes do not poison the Cairo context or subsequent elements")
	{
		for (auto shape : { ElementShapeType::Rectangle, ElementShapeType::Ellipse, ElementShapeType::RoundRect })
		{
			ShapeRenderTarget target;
			auto solid = Ptr(GuiSolidBackgroundElement::Create());
			auto border = Ptr(GuiSolidBorderElement::Create());
			auto gradient = Ptr(GuiGradientBackgroundElement::Create());
			solid->SetShape({ shape,12,6 });
			border->SetShape(solid->GetShape());
			gradient->SetShape(solid->GetShape());
			for (auto bounds : { Rect(8, 8, 8, 24), Rect(8, 8, 24, 8), Rect(24, 8, 8, 24) })
			{
				target.Draw(solid.Obj(), bounds);
				target.Draw(border.Obj(), bounds);
				target.Draw(gradient.Obj(), bounds);
			}
			TEST_ASSERT(target.Pixel(16, 16) == 0);
			solid->SetShape({ ElementShapeType::Rectangle });
			solid->SetColor(Color(255, 0, 0));
			target.Draw(solid.Obj());
			TEST_ASSERT(target.Pixel(32, 32) == 0xFFFF0000);
		}
	});

	TEST_CASE(L"Shaped gradients preserve clipping alpha and the caller transform")
	{
		ShapeRenderTarget target;
		auto element = Ptr(GuiGradientBackgroundElement::Create());
		element->SetShape({ ElementShapeType::Ellipse });
		element->SetColors(Color(255, 0, 0, 128), Color(255, 0, 0, 128));
		cairo_rectangle(target.context, 8, 8, 24, 48);
		cairo_clip(target.context);
		target.Draw(element.Obj());
		TEST_ASSERT(target.Pixel(24, 32) >> 24 == 128);
		TEST_ASSERT(target.Pixel(40, 32) == 0);
		cairo_translate(target.context, 2, 3);
		cairo_scale(target.context, 1.5, 1.25);
		cairo_matrix_t before, after;
		cairo_get_matrix(target.context, &before);
		element->SetShape({ ElementShapeType::RoundRect,12,3 });
		target.Draw(element.Obj());
		cairo_get_matrix(target.context, &after);
		TEST_ASSERT(before.xx == after.xx && before.yx == after.yx && before.xy == after.xy && before.yy == after.yy && before.x0 == after.x0 && before.y0 == after.y0);
		TEST_ASSERT(target.Pixel(40, 32) == 0);
	});
}

vint shapeTestArgc = 0;
char** shapeTestArgv = nullptr;
vint shapeTestResult = 0;

void GuiMain()
{
	shapeTestResult = unittest::UnitTest::RunAndDisposeTests(shapeTestArgc, shapeTestArgv);
}

int main(int argc, char* argv[])
{
	shapeTestArgc = argc;
	shapeTestArgv = argv;
	auto result = SetupWGacRenderer();
	return result == 0 ? shapeTestResult : result;
}
