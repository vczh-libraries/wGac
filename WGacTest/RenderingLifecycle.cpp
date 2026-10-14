#include "GacUI.h"
#include "WGacGacView.h"
#include <cstring>

using namespace vl;
using namespace vl::presentation::wayland;

namespace
{
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
}

TEST_FILE
{
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

int main(int argc, char* argv[])
{
    auto result = unittest::UnitTest::RunAndDisposeTests(argc, argv);
    FinalizeGlobalStorage();
    return result;
}
