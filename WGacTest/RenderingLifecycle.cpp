#include "GacUI.h"
#include "WGacGacView.h"
#include "WGacNativeWindow.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

using namespace vl;
using namespace vl::presentation;
using namespace vl::presentation::wayland;

class ShmConnection
{
public:
    wl_display* display = nullptr;
    wl_registry* registry = nullptr;
    wl_shm* shm = nullptr;

    ShmConnection()
    {
        display = wl_display_connect(nullptr);
        TEST_ASSERT(display);
        registry = wl_display_get_registry(display);
        static const wl_registry_listener listener = {
            [](void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t)
            {
                if (std::strcmp(interface, wl_shm_interface.name) == 0)
                {
                    static_cast<ShmConnection*>(data)->shm = static_cast<wl_shm*>(
                        wl_registry_bind(registry, name, &wl_shm_interface, 1));
                }
            },
            [](void*, wl_registry*, uint32_t) {},
        };
        wl_registry_add_listener(registry, &listener, this);
        TEST_ASSERT(wl_display_roundtrip(display) >= 0);
        TEST_ASSERT(shm);
    }

    ~ShmConnection()
    {
        wl_shm_destroy(shm);
        wl_registry_destroy(registry);
        wl_display_disconnect(display);
    }
};

// Inspect requests actually marshalled to Wayland, including commits made by
// libdecor callbacks. No compositor timing assumptions are needed for this check.
class SurfaceCommitTrace
{
    FILE* trace;
    int savedStderr;

public:
    SurfaceCommitTrace()
    {
        trace = std::tmpfile();
        TEST_ASSERT(trace);
        std::fflush(stderr);
        savedStderr = dup(STDERR_FILENO);
        TEST_ASSERT(savedStderr >= 0);
        TEST_ASSERT(dup2(fileno(trace), STDERR_FILENO) >= 0);
    }

    ~SurfaceCommitTrace()
    {
        std::fflush(stderr);
        dup2(savedStderr, STDERR_FILENO);
        close(savedStderr);
        std::fclose(trace);
    }

    vint Count(wl_surface* surface)
    {
        std::fflush(stderr);
        std::rewind(trace);
        char currentName[80];
        char legacyName[80];
        auto id = wl_proxy_get_id(reinterpret_cast<wl_proxy*>(surface));
        std::snprintf(currentName, sizeof(currentName), "wl_surface#%u.commit()", id);
        std::snprintf(legacyName, sizeof(legacyName), "wl_surface@%u.commit()", id);
        char line[4096];
        vint count = 0;
        while (std::fgets(line, sizeof(line), trace))
        {
            if (std::strstr(line, "->") &&
                (std::strstr(line, currentName) || std::strstr(line, legacyName)))
            {
                count++;
            }
        }
        std::fseek(trace, 0, SEEK_END);
        return count;
    }
};

class InitialPaintListener : public INativeWindowListener
{
public:
    bool requested = false;

    void Paint() override
    {
        requested = true;
    }
};

void DrawWindow(WGacNativeWindow& window)
{
    auto view = window.GetGacView();
    view->StartRendering();
    TEST_ASSERT(view->GetCairoContext());
    cairo_set_source_rgb(view->GetCairoContext(), 0, 0, 0);
    cairo_paint(view->GetCairoContext());
    view->StopRendering();
    window.CommitBuffer();
}

TEST_FILE
{
    TEST_CASE(L"Window geometry waits for its matching content buffer")
    {
        SurfaceCommitTrace trace;
        WaylandDisplay display;
        TEST_ASSERT(display.Connect());
        InitialPaintListener listener;
        WGacNativeWindow window(&display, INativeWindow::WindowMode::Normal);
        TEST_ASSERT(window.Create());
        window.EnableCustomFrameMode();
        window.SetClientSize(NativeSize(320, 240));
        window.InstallListener(&listener);
        window.Show();
        for (vint i = 0; i < 200 && !listener.requested; i++)
        {
            TEST_ASSERT(display.DispatchWithTimeout(25) >= 0);
        }
        TEST_ASSERT(listener.requested);
        DrawWindow(window);
        auto commits = trace.Count(window.GetSurface());
        TEST_ASSERT(commits > 0);

        for (auto size : { NativeSize(480, 320), NativeSize(240, 180) })
        {
            window.SetClientSize(size);
            // Property changes and libdecor's decoration callback must not
            // apply the pending geometry against the previously attached image.
            window.SuggestMinClientSize(NativeSize(100, 100));
            window.SetTitle(L"Pending resize");
            window.DisableCustomFrameMode();
            window.EnableCustomFrameMode();
            window.RedrawContent();
            TEST_ASSERT(trace.Count(window.GetSurface()) == commits);

            DrawWindow(window);
            TEST_ASSERT(trace.Count(window.GetSurface()) == ++commits);
            TEST_ASSERT(window.GetClientSize() == size);
        }

        auto view = window.GetGacView();
        view->StartRendering();
        window.SetClientSize(NativeSize(360, 260));
        view->StopRendering();
        window.CommitBuffer();
        TEST_ASSERT(trace.Count(window.GetSurface()) == commits);
        DrawWindow(window);
        TEST_ASSERT(trace.Count(window.GetSurface()) == ++commits);

        // Once matching content is attached, decoration-only commits must work.
        window.SetTitle(L"Resize complete");
        TEST_ASSERT(trace.Count(window.GetSurface()) > commits);
        window.Hide(false);
        window.UninstallListener(&listener);
        TEST_ASSERT(display.Roundtrip() >= 0);
    });

    TEST_CASE(L"A delayed tooltip with an unmapped popup parent preserves the connection")
    {
        WaylandDisplay display;
        TEST_ASSERT(display.Connect());
        {
            WGacNativeWindow parent(&display, INativeWindow::WindowMode::Menu);
            WGacNativeWindow tooltip(&display, INativeWindow::WindowMode::Tooltip);
            TEST_ASSERT(parent.Create());
            TEST_ASSERT(tooltip.Create());
            tooltip.SetParent(&parent);

            // A menu can close before its tooltip's delayed Show callback runs.
            // The parent has no xdg role, so the tooltip must not create one.
            tooltip.Show();
            TEST_ASSERT(wl_display_roundtrip(display.GetDisplay()) >= 0);
            TEST_ASSERT(wl_display_roundtrip(display.GetDisplay()) >= 0);
            TEST_ASSERT(wl_display_get_error(display.GetDisplay()) == 0);
            tooltip.Hide(false);
        }
        TEST_ASSERT(wl_display_roundtrip(display.GetDisplay()) >= 0);
    });

    TEST_CASE(L"Resizing during a draw retains its buffer and Cairo backing memory")
    {
        ShmConnection connection;
        WaylandBufferPool pool(connection.shm);
        TEST_ASSERT(pool.Resize(64, 32));
        WGacView view(nullptr, &pool);
        view.StartRendering();
        auto* original = view.GetCurrentBuffer();
        auto* context = view.GetCairoContext();
        TEST_ASSERT(original && context);

        // Hosted layout can resize the native window between Start/StopRendering.
        TEST_ASSERT(pool.Resize(128, 96));
        TEST_ASSERT(pool.Resize(96, 48));
        TEST_ASSERT(view.GetCurrentBuffer() == original);
        TEST_ASSERT(original->GetWidth() == 64);
        TEST_ASSERT(original->GetHeight() == 32);
        TEST_ASSERT(pool.GetWidth() == 96);
        TEST_ASSERT(pool.GetHeight() == 48);
        TEST_ASSERT(pool.GetNextBuffer() != original);

        cairo_set_source_rgb(context, 1, 0, 0);
        cairo_paint(context);
        TEST_ASSERT(cairo_status(context) == CAIRO_STATUS_SUCCESS);
        view.StopRendering();
        TEST_ASSERT(static_cast<uint32_t*>(original->GetData())[0] == 0xFFFF0000);
        TEST_ASSERT(view.GetCairoContext() == nullptr);
        TEST_ASSERT(!view.IsRendering());

        view.StartRendering();
        TEST_ASSERT(view.GetCurrentBuffer() == pool.GetNextBuffer());
        TEST_ASSERT(view.GetCurrentBuffer()->GetWidth() == 96);
        TEST_ASSERT(view.GetCurrentBuffer()->GetHeight() == 48);
        cairo_set_source_rgb(view.GetCairoContext(), 0, 0, 1);
        cairo_paint(view.GetCairoContext());
        view.StopRendering();
        TEST_ASSERT(static_cast<uint32_t*>(view.GetCurrentBuffer()->GetData())[0] == 0xFF0000FF);
    });
}

void GuiMain()
{
}

int main(int argc, char* argv[])
{
    // libwayland reads this once, when the first connection is constructed.
    setenv("WAYLAND_DEBUG", "client", 1);
    auto result = unittest::UnitTest::RunAndDisposeTests(argc, argv);
    FinalizeGlobalStorage();
    return result;
}
