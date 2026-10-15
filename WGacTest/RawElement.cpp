#include "GuiWaylandElement.h"
#include "WGacNativeWindow.h"
#include "WGacGacView.h"
#include "../WGacShared/FullControlTestRawElement.h"
#include "../WGacShared/FullControlTestRawElementRemote.h"
#include <cstring>

using namespace vl;
using namespace vl::collections;
using namespace vl::presentation;
using namespace vl::presentation::elements;
using namespace vl::presentation::elements::wgac;
using namespace vl::presentation::compositions;
using namespace vl::presentation::remoteprotocol;
using namespace vl::presentation::remote_renderer;

class RawCairoTarget : public IWGacRenderTarget
{
public:
	cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 600, 600);
	cairo_t* context = nullptr;

	~RawCairoTarget() { cairo_surface_destroy(surface); }
	cairo_t* GetCairoContext() override { return context; }
	Size GetCanvasSize() override { return { 300,300 }; }
	void StartRenderingOnNativeWindow() override
	{
		context = cairo_create(surface);
		cairo_set_operator(context, CAIRO_OPERATOR_CLEAR);
		cairo_paint(context);
		cairo_set_operator(context, CAIRO_OPERATOR_OVER);
	}
	RenderTargetFailure StopRenderingOnNativeWindow() override
	{
		TEST_ASSERT(cairo_status(context) == CAIRO_STATUS_SUCCESS);
		cairo_destroy(context);
		context = nullptr;
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
};

vuint64_t PixelHash(cairo_surface_t* surface)
{
	cairo_surface_flush(surface);
	auto pixels = cairo_image_surface_get_data(surface);
	auto size = cairo_image_surface_get_stride(surface) * cairo_image_surface_get_height(surface);
	vuint64_t hash = 14695981039346656037ULL;
	for (vint i = 0; i < size; i++) hash = (hash ^ pixels[i]) * 1099511628211ULL;
	return hash;
}

class RawNativeEvents : public Object, public IGuiRemoteProtocolEvents
{
public:
#define EVENT_NOREQ(NAME, REQUEST) void On ## NAME() override {}
#define EVENT_REQ(NAME, REQUEST) void On ## NAME(const REQUEST&) override {}
#define EVENT_HANDLER(NAME, REQUEST, REQTAG, ...) EVENT_ ## REQTAG(NAME, REQUEST)
	GACUI_REMOTEPROTOCOL_EVENTS(EVENT_HANDLER)
#undef EVENT_HANDLER
#undef EVENT_REQ
#undef EVENT_NOREQ
#define MESSAGE_NORES(NAME, RESPONSE)
#define MESSAGE_RES(NAME, RESPONSE) void Respond ## NAME(vint, const RESPONSE&) override {}
#define MESSAGE_HANDLER(NAME, REQUEST, RESPONSE, REQTAG, RESTAG, ...) MESSAGE_ ## RESTAG(NAME, RESPONSE)
	GACUI_REMOTEPROTOCOL_MESSAGES(MESSAGE_HANDLER)
#undef MESSAGE_HANDLER
#undef MESSAGE_RES
#undef MESSAGE_NORES
};

class RawNativeRenderer : public GuiRemoteRendererSingle
{
public:
	RawNativeRenderer(INativeWindow* nativeWindow) : GuiRemoteRendererSingle(true) { window = nativeWindow; }
	using GuiRemoteRendererSingle::AfterClosing;
	using GuiRemoteRendererSingle::DisconnectFromCore;
	using GuiRemoteRendererSingle::UpdateRenderTarget;
	using GuiRemoteRendererSingle::ForceRender;
	using GuiRemoteRendererSingle::needRefresh;
};

TEST_FILE
{
	TEST_CASE(L"Raw factory preserves concrete identity and common state in native and hosted modes")
	{
		auto element = GetGuiGraphicsResourceManager()->CreateRawElement().Cast<GuiWaylandElement>();
		TEST_ASSERT(element);
		TEST_ASSERT(dynamic_cast<GuiRawElementBase*>(element.Obj()));
		TEST_ASSERT(dynamic_cast<GuiWaylandElementRenderer*>(element->GetRenderer()));
		TEST_ASSERT(element->GetId() == L"" && element->GetData() == L"");
		TEST_ASSERT(element->GetRenderer()->GetMinSize() == Size(0, 0));
		auto direct = Ptr(GuiWaylandElement::Create());
		TEST_ASSERT(direct->GetRenderer()->GetFactory() == element->GetRenderer()->GetFactory());
		GuiBoundsComposition composition;
		composition.SetOwnedElement(element);
		TEST_ASSERT(element->GetOwnerComposition() == &composition);
		element->SetId(L"duplicate");
		direct->SetId(L"duplicate");
		element->SetData(L"12:34:56");
		element->SetData(L"12:34:56");
		TEST_ASSERT(element->GetId() == direct->GetId() && element->GetData() == L"12:34:56");
		element->SetId(L"");
		element->SetData(L"");
		TEST_ASSERT(element->GetId() == L"" && element->GetData() == L"");
		composition.SetOwnedElement(nullptr);
		TEST_ASSERT(!element->GetOwnerComposition());
	});

	TEST_CASE(L"Raw Cairo callback preserves clipping, transform, drawing state and current path")
	{
		auto element = Ptr(GuiWaylandElement::Create());
		auto renderer = element->GetRenderer();
		RawCairoTarget target;
		renderer->SetRenderTarget(&target);
		vint paints = 0;
		auto handler = element->Rendering.AttachLambda([&](GuiGraphicsComposition*, GuiWaylandElementEventArgs& args)
		{
			paints++;
			TEST_ASSERT(args.element == element.Obj() && args.context == target.GetCairoContext());
			TEST_ASSERT(args.bounds == Rect(10, 10, 110, 110));
			cairo_set_source_rgb(args.context, 1, 0, 0);
			cairo_paint(args.context);
			cairo_translate(args.context, 77, 88);
			cairo_scale(args.context, 3, 4);
			cairo_set_line_width(args.context, 23);
			cairo_rectangle(args.context, 0, 0, 1, 1);
			cairo_clip(args.context);
			cairo_move_to(args.context, 50, 60);
			cairo_line_to(args.context, 70, 80);
		});
		// Attachment can precede a pass; no native pointer is available yet.
		renderer->Render({ 10,10,110,110 });
		TEST_ASSERT(paints == 0);
		for (vint scale : { 1,2 })
		{
			target.StartRendering();
			auto context = target.GetCairoContext();
			cairo_scale(context, scale, scale);
			cairo_translate(context, 5, 6);
			cairo_set_line_width(context, 7);
			cairo_set_source_rgb(context, 0, 1, 0);
			cairo_move_to(context, 1, 2);
			cairo_line_to(context, 3, 4);
			renderer->Render({ 10,10,110,110 });
			TEST_ASSERT(cairo_get_line_width(context) == 7);
			cairo_matrix_t matrix;
			cairo_get_matrix(context, &matrix);
			TEST_ASSERT(matrix.xx == scale && matrix.yy == scale && matrix.x0 == 5 * scale && matrix.y0 == 6 * scale);
			auto path = cairo_copy_path(context);
			TEST_ASSERT(path->num_data == 4);
			TEST_ASSERT(path->data[1].point.x == 1 && path->data[1].point.y == 2);
			TEST_ASSERT(path->data[3].point.x == 3 && path->data[3].point.y == 4);
			cairo_path_destroy(path);
			// A neighboring draw must still use the original green source and unrestricted clip.
			cairo_new_path(context);
			cairo_rectangle(context, 150, 150, 10, 10);
			cairo_fill(context);
			cairo_surface_flush(target.surface);
			auto stride = cairo_image_surface_get_stride(target.surface) / 4;
			auto pixels = reinterpret_cast<vuint32_t*>(cairo_image_surface_get_data(target.surface));
			TEST_ASSERT(pixels[30 * scale * stride + 30 * scale] == 0xFFFF0000);
			TEST_ASSERT(pixels[160 * scale * stride + 160 * scale] == 0xFF00FF00);
			TEST_ASSERT(pixels[5 * scale * stride + 5 * scale] == 0);
			auto count = paints;
			renderer->Render({ 400,400,450,450 });
			TEST_ASSERT(paints == count);
			target.StopRendering();
			TEST_ASSERT(!target.GetCairoContext());
		}
		TEST_ASSERT(paints == 2);
		renderer->SetRenderTarget(nullptr);
		renderer->Render({ 10,10,110,110 });
		TEST_ASSERT(paints == 2);
		element->Rendering.Detach(handler);
	});

	TEST_CASE(L"Clock paints only valid received time and reattaches after target replacement")
	{
		auto element = Ptr(GuiWaylandElement::Create());
		auto attachment = demo::AttachFullControlTestRawElement(element);
		RawCairoTarget target;
		element->GetRenderer()->SetRenderTarget(&target);
		auto draw = [&]()
		{
			target.StartRendering();
			element->GetRenderer()->Render({ 0,0,240,240 });
			target.StopRendering();
			return PixelHash(target.surface);
		};
		auto blank = draw();
		element->SetId(L"FullControlTest.Clock");
		for (auto data : { L"", L"24:00:00", L"12:60:00", L"12:00:60", L"1:23:45", L"aa:bb:cc" })
		{
			element->SetData(data);
			TEST_ASSERT(draw() == blank);
		}
		element->SetData(L"03:00:00");
		auto three = draw();
		TEST_ASSERT(three != blank && draw() == three);
		auto stride = cairo_image_surface_get_stride(target.surface) / 4;
		auto pixels = reinterpret_cast<vuint32_t*>(cairo_image_surface_get_data(target.surface));
		// At three o'clock the cyan hour points right and the yellow second points up.
		TEST_ASSERT(pixels[120 * stride + 170] == 0xFF00FFFF);
		TEST_ASSERT(pixels[50 * stride + 120] == 0xFFFFFF00);
		TEST_ASSERT(pixels[160 * stride + 80] == 0);
		element->SetData(L"06:30:45");
		TEST_ASSERT(draw() != three && draw() != blank);
		element->SetId(L"other");
		TEST_ASSERT(draw() == blank);
		element->SetId(L"FullControlTest.Clock");
		element->SetData(L"03:00:00");
		element->GetRenderer()->SetRenderTarget(nullptr);
		RawCairoTarget replacement;
		element->GetRenderer()->SetRenderTarget(&replacement);
		attachment = nullptr;
		attachment = demo::AttachFullControlTestRawElement(element);
		replacement.StartRendering();
		element->GetRenderer()->Render({ 0,0,240,240 });
		replacement.StopRendering();
		TEST_ASSERT(PixelHash(replacement.surface) == three);
		attachment = nullptr;
		replacement.StartRendering();
		element->GetRenderer()->Render({ 0,0,240,240 });
		replacement.StopRendering();
		TEST_ASSERT(PixelHash(replacement.surface) == blank);
		element->GetRenderer()->SetRenderTarget(nullptr);
	});

	TEST_CASE(L"Native remote raw state, collection membership, data-only repaint and retained lifetime")
	{
		// GuiRemoteRendererSingle owns a real native window, rather than a hosted virtual window.
		if (GetHostedApplication()) return;
		auto service = GetCurrentController()->WindowService();
		auto native = service->CreateNativeWindow(INativeWindow::Normal);
		native->SetClientSize({ 300,300 });
		auto view = dynamic_cast<wayland::WGacNativeWindow*>(native)->GetGacView();
		RawNativeEvents responses;
		RawNativeRenderer renderer(native);
		renderer.Initialize(&responses);
		auto attachment = demo::AttachFullControlTestRawElements(renderer);
		vint installs = 0, changes = 0, uninstalls = 0, paints = 0;
		Ptr<GuiWaylandElement> retained;
		WString expectedId, expectedData;
		renderer.RawElementInstalled.Add([&](const GuiRemoteRawElementEventArgs& args)
		{
			TEST_ASSERT(renderer.GetRawElements().Contains(args.element.Obj()));
			TEST_ASSERT(args.element->GetId() == L"" && args.element->GetData() == L"");
			retained = args.element.Cast<GuiWaylandElement>();
			TEST_ASSERT(retained);
			installs++;
			retained->Rendering.AttachLambda([&](GuiGraphicsComposition*, GuiWaylandElementEventArgs& args)
			{
				TEST_ASSERT(args.context == view->GetCairoContext());
				paints++;
			});
		});
		renderer.RawElementChanged.Add([&](const GuiRemoteRawElementEventArgs& args)
		{
			TEST_ASSERT(args.element->GetId() == expectedId && args.element->GetData() == expectedData);
			changes++;
		});
		renderer.RawElementUninstalled.Add([&](const GuiRemoteRawElementEventArgs& args)
		{
			TEST_ASSERT(renderer.GetRawElements().Contains(args.element.Obj()));
			uninstalls++;
		});
		auto create = Ptr(new List<RendererCreation>);
		create->Add({ 41,RendererType::SolidBackground });
		create->Add({ 42,RendererType::Raw });
		renderer.RequestRendererCreated(create);
		auto first = retained;
		auto update = [&](vint id, const WString& name, const WString& data)
		{
			expectedId = name;
			expectedData = data;
			ElementBeginRendering begin;
			begin.updatedElements = Ptr(new List<OrdinaryElementDescVariant>);
			begin.updatedElements->Add(ElementDesc_Raw{ id,name,data });
			renderer.RequestRendererBeginRendering(begin);
		};
		update(42, L"", L"");
		TEST_ASSERT(changes == 0);
		update(42, L"FullControlTest.Clock", L"03:00:00");
		TEST_ASSERT(changes == 1 && renderer.needRefresh);
		auto dom = Ptr(new RenderingDom);
		dom->id = 1;
		dom->content.element = 41;
		dom->content.bounds = { 0,0,600,600 };
		dom->content.validArea = dom->content.bounds;
		auto clockDom = Ptr(new RenderingDom);
		clockDom->id = 2;
		clockDom->content.element = 42;
		clockDom->content.bounds = { 0,0,240,240 };
		clockDom->content.validArea = clockDom->content.bounds;
		dom->children = Ptr(new List<Ptr<RenderingDom>>);
		dom->children->Add(clockDom);
		renderer.RequestRendererRenderDom(dom);
		renderer.RequestRendererEndRendering(1);
		TEST_ASSERT(paints > 0 && !view->GetCairoContext());
		auto pixels = PixelHash(view->GetCurrentBuffer()->GetCairoSurface());
		auto count = paints;
		update(42, L"FullControlTest.Clock", L"06:30:45");
		renderer.RequestRendererEndRendering(2);
		TEST_ASSERT(paints > count && PixelHash(view->GetCurrentBuffer()->GetCairoSurface()) != pixels);
		count = paints;
		update(42, L"other", L"06:30:45");
		TEST_ASSERT(!renderer.needRefresh);
		renderer.RequestRendererEndRendering(3);
		TEST_ASSERT(paints == count && changes == 3);
		update(42, L"other", L"06:30:45");
		TEST_ASSERT(changes == 3);
		update(42, L"FullControlTest.Clock", L"03:00:00");
		attachment = nullptr;
		attachment = demo::AttachFullControlTestRawElements(renderer);
		renderer.RequestRendererEndRendering(4);
		TEST_ASSERT(PixelHash(view->GetCurrentBuffer()->GetCairoSurface()) == pixels);
		create->Clear();
		create->Add({ 43,RendererType::Raw });
		renderer.RequestRendererCreated(create);
		update(43, L"FullControlTest.Clock", L"03:00:00");
		TEST_ASSERT(renderer.GetRawElements().Count() == 2);
		update(43, L"", L"");
		renderer.UpdateRenderTarget(nullptr);
		GetGuiGraphicsResourceManager()->RecreateRenderTarget(native);
		renderer.UpdateRenderTarget(GetGuiGraphicsResourceManager()->GetRenderTarget(native));
		native->SetClientSize({ 400,360 });
		renderer.ForceRender();
		TEST_ASSERT(renderer.GetRawElements()[0] == first);
		create->Clear();
		create->Add({ 42,RendererType::Raw });
		renderer.RequestRendererCreated(create);
		TEST_ASSERT(installs == 3 && uninstalls == 1 && retained != first);
		count = paints;
		first->GetRenderer()->Render({ 0,0,240,240 });
		TEST_ASSERT(paints == count);
		auto destroyed = Ptr(new List<vint>);
		destroyed->Add(43);
		renderer.RequestRendererDestroyed(destroyed);
		TEST_ASSERT(uninstalls == 2);
		update(42, L"FullControlTest.Clock", L"03:00:00");
		renderer.RequestRendererEndRendering(5);
		pixels = PixelHash(view->GetCurrentBuffer()->GetCairoSurface());
		count = paints;
		renderer.DisconnectFromCore();
		TEST_ASSERT(renderer.GetRawElements().Count() == 1 && uninstalls == 2);
		renderer.ForceRender();
		TEST_ASSERT(paints > count && PixelHash(view->GetCurrentBuffer()->GetCairoSurface()) == pixels);
		count = paints;
		renderer.RetainByFatalError(L"Raw clock retained-picture test");
		TEST_ASSERT(paints > count && !view->GetCairoContext());
		TEST_ASSERT(renderer.GetRawElements()[0] == retained && retained->GetData() == L"03:00:00");
		TEST_ASSERT(uninstalls == 2);
		renderer.AfterClosing();
		TEST_ASSERT(renderer.GetRawElements().Count() == 0 && uninstalls == 3);
		attachment = nullptr;
		service->DestroyNativeWindow(native);
		count = paints;
		retained->GetRenderer()->Render({ 0,0,240,240 });
		TEST_ASSERT(paints == count);
	});
}

int rawTestArgc = 0;
char** rawTestArgv = nullptr;
int rawTestResult = 0;

void GuiMain()
{
	rawTestResult = unittest::UnitTest::RunAndDisposeTests(rawTestArgc, rawTestArgv);
}

int main(int argc, char* argv[])
{
	auto hosted = argc > 1 && std::strcmp(argv[argc - 1], "--hosted") == 0;
	rawTestArgc = argc - (hosted ? 1 : 0);
	rawTestArgv = argv;
	auto result = hosted ? SetupWGacHostedRenderer() : SetupWGacRenderer();
	return result == 0 ? rawTestResult : result;
}
