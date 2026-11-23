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

vint CountPixels(cairo_surface_t* surface, Rect area, vuint32_t color)
{
	cairo_surface_flush(surface);
	auto pixels = reinterpret_cast<vuint32_t*>(cairo_image_surface_get_data(surface));
	auto stride = cairo_image_surface_get_stride(surface) / 4;
	vint count = 0;
	for (vint y = area.y1; y < area.y2; y++)
	{
		for (vint x = area.x1; x < area.x2; x++)
		{
			if (pixels[y * stride + x] == color) count++;
		}
	}
	return count;
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

class RawInterpretorState : public Object
{
public:
	vint							created = 0;
	vint							initialized = 0;
	vint							finalized = 0;
	vint							destroyed = 0;
	vint							attachedAtDestruction = 0;
	vint							painted = 0;
	List<GuiRawElementBase*>		live;
	List<Pair<WString, WString>>	initialValues;
};

class CountingRawInterpretor : public Object, public IGuiGraphicsRawInterpretor
{
protected:
	Ptr<RawInterpretorState>		state;
	GuiRawElementBase*				element = nullptr;
	Ptr<IGuiGraphicsEventHandler>	handler;
public:
	CountingRawInterpretor(Ptr<RawInterpretorState> _state) : state(_state)
	{
		state->created++;
	}

	~CountingRawInterpretor()
	{
		if (handler) state->attachedAtDestruction++;
		state->destroyed++;
	}

	void Initialize(IGuiGraphicsElement* value) override
	{
		element = dynamic_cast<GuiRawElementBase*>(value);
		state->initialized++;
		TEST_ASSERT(!state->live.Contains(element));
		state->live.Add(element);
		state->initialValues.Add({ element->GetInterpretor(), element->GetData() });
		handler = static_cast<GuiWaylandElement*>(element)->Rendering.AttachLambda([this](GuiGraphicsComposition*, GuiWaylandElementEventArgs&) { state->painted++; });
	}

	void Finalize() override
	{
		TEST_ASSERT(state->live.Remove(element));
		TEST_ASSERT(static_cast<GuiWaylandElement*>(element)->Rendering.Detach(handler));
		handler = nullptr;
		state->finalized++;
	}
};

class CountingRawInterpretorFactory : public Object, public IGuiGraphicsRawInterpretorFactory
{
protected:
	Ptr<RawInterpretorState> state;
public:
	CountingRawInterpretorFactory(Ptr<RawInterpretorState> _state) : state(_state) {}
	IGuiGraphicsRawInterpretor* Create() override { return new CountingRawInterpretor(state); }
};

TEST_FILE
{
	TEST_CASE(L"Raw interpretor selection, hosted registry forwarding and exactly-once lifetime")
	{
		auto manager = GetGuiGraphicsResourceManager();
		auto state = Ptr(new RawInterpretorState);
		auto factory = Ptr(new CountingRawInterpretorFactory(state));
		{
			GuiHostedController controller(GetCurrentController());
			GuiHostedGraphicsResourceManager hosted(&controller, manager);
			hosted.RegisterRawInterpretorFactory(L"WaylandTest::A", factory);
			manager->RegisterRawInterpretorFactory(L"WaylandTest::B", factory);
			TEST_ASSERT(manager->GetRawInterpretorFactory(L"WaylandTest::A") == factory.Obj());
			TEST_ASSERT(hosted.GetRawInterpretorFactory(L"WaylandTest::B") == factory.Obj());
			TEST_ASSERT(!hosted.GetRawInterpretorFactory(L"missing"));
			TEST_ERROR(hosted.RegisterRawInterpretorFactory(L"WaylandTest::A", factory));
			TEST_ASSERT(hosted.CreateRawElement().Cast<GuiRawElementBase>());
		}
		{
			auto raw = manager->CreateRawElement().Cast<GuiRawElementBase>();
			TEST_ASSERT(raw->GetInterpretor() == L"" && !raw->GetOwnerComposition());
			raw->SetId(L"WaylandTest::A");
			raw->SetData(L"before");
			TEST_ASSERT(state->created == 0);
			raw->SetInterpretorAndData(L"WaylandTest::A", L"first");
			TEST_ASSERT(state->created == 1 && state->initialized == 1);
			TEST_ASSERT(state->initialValues[0].key == L"WaylandTest::A" && state->initialValues[0].value == L"first");
			raw->SetData(L"data only");
			raw->SetId(L"");
			raw->SetInterpretor(L"WaylandTest::A");
			raw->SetInterpretorAndData(L"WaylandTest::A", L"data only");
			TEST_ASSERT(state->created == 1 && state->finalized == 0);
			raw->SetInterpretorAndData(L"WaylandTest::B", L"second");
			TEST_ASSERT(state->created == 2 && state->initialized == 2 && state->finalized == 1 && state->destroyed == 1);
			TEST_ASSERT(state->initialValues[1].key == L"WaylandTest::B" && state->initialValues[1].value == L"second");
			raw->SetInterpretor(L"");
			TEST_ASSERT(state->finalized == 2 && state->destroyed == 2);
			raw->SetInterpretor(L"WaylandTest::A");
			raw->SetInterpretor(L"missing");
			TEST_ASSERT(raw->GetInterpretor() == L"missing");
			TEST_ASSERT(state->created == 3 && state->finalized == 3 && state->destroyed == 3);
			manager->RegisterRawInterpretorFactory(L"missing", factory);
			raw->SetData(L"no late retry");
			raw->SetInterpretor(L"missing");
			TEST_ASSERT(state->created == 3);
			raw->SetInterpretor(L"WaylandTest::A");
			{
				auto other = manager->CreateRawElement().Cast<GuiRawElementBase>();
				other->SetInterpretor(L"WaylandTest::A");
				TEST_ASSERT(state->created == 5 && state->live.Count() == 2);
				TEST_ASSERT(state->live[0] != state->live[1]);
			}
			TEST_ASSERT(state->finalized == 4 && state->destroyed == 4 && state->live.Count() == 1);
		}
		// There was no composition or target to detach the handler before final release.
		TEST_ASSERT(state->created == 5 && state->initialized == 5 && state->finalized == 5 && state->destroyed == 5);
		TEST_ASSERT(state->live.Count() == 0);

		RawCairoTarget target;
		RawCairoTarget replacement;
		{
			auto raw = manager->CreateRawElement().Cast<GuiWaylandElement>();
			auto renderer = raw->GetRenderer();
			renderer->SetRenderTarget(&target);
			raw->SetInterpretor(L"WaylandTest::A");
			target.StartRendering();
			renderer->Render({ 0,0,200,200 });
			target.StopRendering();
			TEST_ASSERT(state->painted == 1 && state->created == 6);
			renderer->SetRenderTarget(nullptr);
			renderer->SetRenderTarget(&replacement);
			replacement.StartRendering();
			renderer->Render({ 0,0,200,200 });
			replacement.StopRendering();
			TEST_ASSERT(state->painted == 2 && state->created == 6 && state->finalized == 5);
			raw->SetInterpretor(L"");
			TEST_ASSERT(state->finalized == 6 && state->destroyed == 6);
			raw->SetInterpretor(L"WaylandTest::B");
			// Final release with a target still detaches before the Rendering event dies.
		}
		TEST_ASSERT(state->created == 7 && state->finalized == 7 && state->destroyed == 7);
		TEST_ASSERT(state->attachedAtDestruction == 0);
	});

	TEST_CASE(L"Showing or activating a visible native window does not reopen it")
	{
		if (GetHostedApplication()) return;
		class VisibilityEvents : public INativeWindowListener
		{
		public:
			vint opened = 0;
			vint closed = 0;
			void Opened() override { opened++; }
			void Closed() override { closed++; }
		} events;
		auto service = GetCurrentController()->WindowService();
		auto native = service->CreateNativeWindow(INativeWindow::Normal);
		native->InstallListener(&events);
		for (vint cycle = 1; cycle <= 2; cycle++)
		{
			native->Show();
			TEST_ASSERT(native->IsVisible() && events.opened == cycle);
			native->Show();
			native->ShowDeactivated();
			native->SetActivate();
			TEST_ASSERT(native->IsVisible() && events.opened == cycle);
			native->Hide(false);
			TEST_ASSERT(!native->IsVisible() && events.closed == cycle);
		}
		native->UninstallListener(&events);
		service->DestroyNativeWindow(native);
	});

	TEST_CASE(L"Raw factory preserves concrete identity and common state in native and hosted modes")
	{
		TEST_ASSERT(GetGuiGraphicsResourceManager()->GetRawInterpretorFactory(L"FullControlTest::Clock"));
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

	TEST_CASE(L"Clock plugin paints received time independently of Id and survives target replacement")
	{
		auto element = Ptr(GuiWaylandElement::Create());
		element->SetInterpretor(L"FullControlTest::Clock");
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
		auto outline = pixels[50 * stride + 121];
		TEST_ASSERT((outline >> 24) > 0 && (outline & 0xFFFFFF) == 0);
		TEST_ASSERT(pixels[160 * stride + 80] == 0);
		element->SetData(L"06:30:45");
		TEST_ASSERT(draw() != three && draw() != blank);
		element->SetId(L"other");
		TEST_ASSERT(draw() != blank);
		element->SetId(L"FullControlTest.Clock");
		element->SetData(L"03:00:00");
		element->GetRenderer()->SetRenderTarget(nullptr);
		RawCairoTarget replacement;
		element->GetRenderer()->SetRenderTarget(&replacement);
		replacement.StartRendering();
		element->GetRenderer()->Render({ 0,0,240,240 });
		replacement.StopRendering();
		TEST_ASSERT(PixelHash(replacement.surface) == three);
		element->SetInterpretor(L"");
		replacement.StartRendering();
		element->GetRenderer()->Render({ 0,0,240,240 });
		replacement.StopRendering();
		TEST_ASSERT(PixelHash(replacement.surface) == blank);
		element->GetRenderer()->SetRenderTarget(nullptr);
	});

	TEST_CASE(L"Manual Touching painter preserves payload geometry, contacts, clipping and independent lifetime")
	{
		auto element = Ptr(GuiWaylandElement::Create());
		auto attachment = demo::AttachFullControlTestTouchingElement(element);
		RawCairoTarget target;
		element->GetRenderer()->SetRenderTarget(&target);
		auto draw = [&](Rect bounds = { 10,10,250,250 })
		{
			target.StartRendering();
			element->GetRenderer()->Render(bounds);
			target.StopRendering();
			return PixelHash(target.surface);
		};
		auto blank = draw();
		element->SetId(L"FullControlTest.Touching");
		TEST_ASSERT(element->GetInterpretor() == L"");
		for (auto data : { L"bad", L"120,120,1,0,-1", L"120,120,1,0,1", L"120,120,1,0,0.5", L"120,120,1,0,0.3333333333333333,7", L"120,120,1,0,0.6666666666666666,7,40", L"120,120,nan,0,0" })
		{
			element->SetData(data);
			TEST_ASSERT(draw() == blank);
		}
		element->SetData(L"120,120,1,0,1,7,40,40");
		auto initial = draw();
		TEST_ASSERT(initial != blank);
		TEST_ASSERT(CountPixels(target.surface, { 51,81,209,179 }, 0xFF87CEEB) == 158 * 98);
		TEST_ASSERT(CountPixels(target.surface, { 51,79,209,81 }, 0xFF000000) == 158 * 2);
		TEST_ASSERT(CountPixels(target.surface, { 30,30,70,70 }, 0xFFFF8800) > 50);
		// Removing the renderer-owned painter must leave the separate manual handler intact.
		element->SetInterpretor(L"FullControlTest::Clock");
		TEST_ASSERT(draw() == initial);
		element->SetInterpretor(L"");
		TEST_ASSERT(draw() == initial);
		element->SetInterpretor(L"unknown Touching painter");
		TEST_ASSERT(draw() == initial);
		element->SetInterpretor(L"");
		// Translation and a quarter-turn at half size: rectangle is now 50 by 80.
		element->SetData(L"100,90,0,0.5,2,7,180,180,8,30,180");
		TEST_ASSERT(draw() != initial);
		TEST_ASSERT(CountPixels(target.surface, { 86,61,134,139 }, 0xFF87CEEB) == 48 * 78);
		TEST_ASSERT(CountPixels(target.surface, { 170,170,210,210 }, 0xFFFF8800) > 50);
		TEST_ASSERT(CountPixels(target.surface, { 20,170,60,210 }, 0xFFFF8800) > 50);
		TEST_ASSERT(CountPixels(target.surface, { 30,30,70,60 }, 0xFFFF8800) == 0);
		element->SetData(L"0,0,1,0,0");
		draw({ 20,30,60,70 });
		TEST_ASSERT(CountPixels(target.surface, { 0,0,300,300 }, 0xFF87CEEB) == 1600);
		attachment = nullptr;
		TEST_ASSERT(draw() == blank);
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
		WString expectedId, expectedInterpretor, expectedData;
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
			TEST_ASSERT(args.element->GetInterpretor() == expectedInterpretor);
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
		auto update = [&](vint id, const WString& name, const WString& data, const WString& interpretor = L"FullControlTest::Clock")
		{
			expectedId = name;
			expectedInterpretor = interpretor;
			expectedData = data;
			ElementBeginRendering begin;
			begin.updatedElements = Ptr(new List<OrdinaryElementDescVariant>);
			begin.updatedElements->Add(ElementDesc_Raw{ .id = id, .rawId = name, .interpretor = interpretor, .data = data });
			ElementBeginRendering decoded;
			ConvertJsonToCustomType(ConvertCustomTypeToJson(begin), decoded);
			renderer.RequestRendererBeginRendering(decoded);
		};
		update(42, L"", L"", L"");
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
		// Only the painter name changes: fixed data, no timer, input, resize or DOM update.
		for (auto name : { L"", L"missing clock", L"FullControlTest::Clock" })
		{
			count = paints;
			update(42, L"FullControlTest.Clock", L"03:00:00", name);
			TEST_ASSERT(renderer.needRefresh);
			renderer.RequestRendererEndRendering(4);
			TEST_ASSERT(paints > count);
			auto actual = PixelHash(view->GetCurrentBuffer()->GetCairoSurface());
			TEST_ASSERT((actual == pixels) == (WString(name) == L"FullControlTest::Clock"));
		}
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
		auto retainedState = Ptr(new RawInterpretorState);
		GetGuiGraphicsResourceManager()->RegisterRawInterpretorFactory(L"WaylandTest::Retained", Ptr(new CountingRawInterpretorFactory(retainedState)));
		retained->SetInterpretor(L"WaylandTest::Retained");
		TEST_ASSERT(retainedState->created == 1 && retainedState->initialized == 1);
		renderer.AfterClosing();
		TEST_ASSERT(renderer.GetRawElements().Count() == 0 && uninstalls == 3);
		TEST_ASSERT(retainedState->finalized == 0 && retainedState->destroyed == 0);
		attachment = nullptr;
		service->DestroyNativeWindow(native);
		count = paints;
		retained->GetRenderer()->Render({ 0,0,240,240 });
		TEST_ASSERT(paints == count);
		retained = nullptr;
		TEST_ASSERT(retainedState->finalized == 1 && retainedState->destroyed == 1);
		TEST_ASSERT(retainedState->attachedAtDestruction == 0);
	});

	TEST_CASE(L"Remote manual Touching reconciles late enumeration, repeated changes, removal and destruction")
	{
		if (GetHostedApplication()) return;
		auto service = GetCurrentController()->WindowService();
		auto native = service->CreateNativeWindow(INativeWindow::Normal);
		native->SetClientSize({ 300,300 });
		auto view = dynamic_cast<wayland::WGacNativeWindow*>(native)->GetGacView();
		RawNativeEvents responses;
		RawNativeRenderer renderer(native);
		renderer.Initialize(&responses);
		auto create = Ptr(new List<RendererCreation>);
		create->Add({ 41,RendererType::SolidBackground });
		create->Add({ 42,RendererType::Raw });
		renderer.RequestRendererCreated(create);
		create->RemoveAt(0);
		auto element = renderer.GetRawElements()[0].Cast<GuiWaylandElement>();
		auto update = [&](const WString& id, const WString& data)
		{
			ElementBeginRendering begin;
			begin.updatedElements = Ptr(new List<OrdinaryElementDescVariant>);
			begin.updatedElements->Add(ElementDesc_Raw{ .id = 42, .rawId = id, .interpretor = L"", .data = data });
			renderer.RequestRendererBeginRendering(begin);
			renderer.RequestRendererEndRendering(1);
		};
		auto dom = Ptr(new RenderingDom);
		dom->id = 1;
		dom->content.element = 41;
		dom->content.bounds = { 0,0,600,600 };
		dom->content.validArea = dom->content.bounds;
		auto touchingDom = Ptr(new RenderingDom);
		touchingDom->id = 2;
		touchingDom->content.element = 42;
		touchingDom->content.bounds = { 10,10,250,250 };
		touchingDom->content.validArea = touchingDom->content.bounds;
		dom->children = Ptr(new List<Ptr<RenderingDom>>);
		dom->children->Add(touchingDom);
		renderer.RequestRendererRenderDom(dom);
		update(L"FullControlTest.Touching", L"120,120,1,0,1,7,40,40");
		auto draw = [&]()
		{
			renderer.ForceRender();
			return PixelHash(view->GetCurrentBuffer()->GetCairoSurface());
		};
		auto blank = draw();
		// Attach after both creation and descriptor delivery.
		auto attachment = demo::AttachFullControlTestRawElements(renderer);
		auto painted = draw();
		TEST_ASSERT(painted != blank && element->GetInterpretor() == L"");
		for (vint i = 0; i < 3; i++)
		{
			update(L"FullControlTest.Touching", L"100,100,0,0.5,1,7,180,180");
			TEST_ASSERT(draw() != painted);
			update(L"FullControlTest.Touching", L"120,120,1,0,1,7,40,40");
			TEST_ASSERT(draw() == painted);
			update(L"FullControlTest.Touching", L"120,120,1,0,1,7,40,40");
			TEST_ASSERT(draw() == painted);
		}
		// ID-based manual attachment goes away; Clock is never attached by ID.
		update(L"FullControlTest.Clock", L"03:00:00");
		TEST_ASSERT(draw() == blank);
		update(L"FullControlTest.Touching", L"120,120,1,0,1,7,40,40");
		TEST_ASSERT(draw() == painted);
		attachment = nullptr;
		TEST_ASSERT(draw() == blank);
		attachment = demo::AttachFullControlTestRawElements(renderer);
		TEST_ASSERT(draw() == painted);
		// Numeric-ID replacement uninstalls the old attachment and installs a new one.
		renderer.RequestRendererCreated(create);
		TEST_ASSERT(renderer.GetRawElements()[0] != element);
		update(L"FullControlTest.Touching", L"120,120,1,0,1,7,40,40");
		TEST_ASSERT(draw() == painted);
		RawCairoTarget target;
		element->GetRenderer()->SetRenderTarget(&target);
		target.StartRendering();
		element->GetRenderer()->Render({ 0,0,240,240 });
		target.StopRendering();
		TEST_ASSERT(CountPixels(target.surface, { 0,0,300,300 }, 0) == 90000);
		element->GetRenderer()->SetRenderTarget(nullptr);
		element = renderer.GetRawElements()[0].Cast<GuiWaylandElement>();
		auto destroyed = Ptr(new List<vint>);
		destroyed->Add(42);
		renderer.RequestRendererDestroyed(destroyed);
		TEST_ASSERT(renderer.GetRawElements().Count() == 0);
		element->GetRenderer()->SetRenderTarget(&target);
		target.StartRendering();
		element->GetRenderer()->Render({ 0,0,240,240 });
		target.StopRendering();
		TEST_ASSERT(CountPixels(target.surface, { 0,0,300,300 }, 0) == 90000);
		element->GetRenderer()->SetRenderTarget(nullptr);
		renderer.AfterClosing();
		attachment = nullptr;
		service->DestroyNativeWindow(native);
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
