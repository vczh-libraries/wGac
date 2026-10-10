#include "Services/WGacInputService.h"
#include <gio/gio.h>
#include <algorithm>
#include <atomic>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

using namespace vl;
using namespace vl::presentation;
using namespace vl::presentation::wayland;

namespace
{
    constexpr const char* PortalName = "org.freedesktop.portal.Desktop";
    constexpr const char* PortalPath = "/org/freedesktop/portal/desktop";
    constexpr const char* ShortcutInterface = "org.freedesktop.portal.GlobalShortcuts";
    constexpr const char* RequestInterface = "org.freedesktop.portal.Request";
    constexpr const char* SessionInterface = "org.freedesktop.portal.Session";

    class Portal
    {
        struct Session
        {
            std::string path;
            std::string sender;
            std::string shortcut;
        };
        GMainContext* context = g_main_context_new();
        GMainLoop* loop = g_main_loop_new(context, false);
        GDBusConnection* connection = nullptr;
        std::thread worker;
        std::mutex mutex;
        std::vector<Session> sessions;
        std::vector<std::string> registered;
        std::vector<guint> objects;
        GDBusNodeInfo* schema;

        static void Call(GDBusConnection*, const gchar* sender, const gchar* path, const gchar* interface,
            const gchar* method, GVariant* parameters, GDBusMethodInvocation* invocation, gpointer data)
        {
            static_cast<Portal*>(data)->OnCall(sender, path, interface, method, parameters, invocation);
        }

        static GVariant* Property(GDBusConnection*, const gchar* sender, const gchar*, const gchar*, const gchar*, GError**, gpointer data)
        {
            auto& registered = static_cast<Portal*>(data)->registered;
            TEST_ASSERT(std::find(registered.begin(), registered.end(), sender) != registered.end());
            return g_variant_new_uint32(1);
        }

        void Export(const std::string& path, int interface)
        {
            static const GDBusInterfaceVTable table = {Call, Property, nullptr, {}};
            objects.push_back(g_dbus_connection_register_object(connection, path.c_str(), schema->interfaces[interface], &table, this, nullptr, nullptr));
        }

        void OnCall(const char* sender, const char* path, const char* interface, const char* method,
            GVariant* parameters, GDBusMethodInvocation* invocation)
        {
            if (std::string(method) == "Register")
            {
                TEST_ASSERT(std::find(registered.begin(), registered.end(), sender) == registered.end());
                registered.push_back(sender);
                g_dbus_method_invocation_return_value(invocation, nullptr);
                return;
            }
            if (std::string(method) == "Close")
            {
                if (std::string(interface) == SessionInterface) closed++;
                else cancelled++;
                g_dbus_method_invocation_return_value(invocation, nullptr);
                return;
            }
            auto options = g_variant_get_child_value(parameters, g_variant_n_children(parameters) - 1);
            const char* token;
            g_variant_lookup(options, "handle_token", "&s", &token);
            std::string caller = sender + 1;
            for (auto& c : caller) if (c == '.') c = '_';
            std::string request = std::string(PortalPath) + "/request/" + caller + "/" + token;
            Export(request, 1);
            GVariantBuilder results;
            g_variant_builder_init(&results, G_VARIANT_TYPE_VARDICT);
            guint response = 0;
            bool send = true;
            if (std::string(method) == "CreateSession")
            {
                TEST_ASSERT(std::find(registered.begin(), registered.end(), sender) != registered.end());
                g_variant_lookup(options, "session_handle_token", "&s", &token);
                std::string session = std::string(PortalPath) + "/session/" + caller + "/" + token;
                Export(session, 2);
                {
                    std::lock_guard lock(mutex);
                    sessions.push_back({session, sender, {}});
                }
                g_variant_builder_add(&results, "{sv}", "session_handle", g_variant_new_string(session.c_str()));
            }
            else
            {
                const char* session;
                const char* parent;
                GVariant* bindings;
                GVariant* extra;
                g_variant_get(parameters, "(&o@a(sa{sv})&s@a{sv})", &session, &bindings, &parent, &extra);
                auto binding = g_variant_get_child_value(bindings, 0);
                const char* id;
                GVariant* properties;
                g_variant_get(binding, "(&s@a{sv})", &id, &properties);
                const char* trigger;
                const char* description;
                TEST_ASSERT(g_variant_lookup(properties, "preferred_trigger", "&s", &trigger));
                TEST_ASSERT(g_variant_lookup(properties, "description", "&s", &description));
                TEST_ASSERT(std::string(id) == trigger);
                TEST_ASSERT(std::string(description).find(trigger) != std::string::npos);
                {
                    std::lock_guard lock(mutex);
                    for (auto& item : sessions) if (item.path == session) item.shortcut = id;
                }
                g_variant_builder_add(&results, "{sv}", "shortcuts", bindings);
                g_variant_unref(properties);
                g_variant_unref(binding);
                g_variant_unref(bindings);
                g_variant_unref(extra);
                response = denied ? 1 : 0;
                send = !pending;
                bound++;
            }
            g_variant_unref(options);
            // Emit before the method reply to exercise the request/response race.
            if (send) g_dbus_connection_emit_signal(connection, sender, request.c_str(), RequestInterface,
                "Response", g_variant_new("(ua{sv})", response, &results), nullptr);
            else g_variant_builder_clear(&results);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(o)", request.c_str()));
        }

    public:
        std::atomic<int> bound = 0;
        std::atomic<int> closed = 0;
        std::atomic<int> cancelled = 0;
        std::atomic<bool> denied = false;
        std::atomic<bool> pending = false;

        Portal()
        {
            schema = g_dbus_node_info_new_for_xml(R"(<node>
                <interface name='org.freedesktop.portal.GlobalShortcuts'>
                    <property name='version' type='u' access='read'/>
                    <method name='CreateSession'><arg type='a{sv}' direction='in'/><arg type='o' direction='out'/></method>
                    <method name='BindShortcuts'><arg type='o' direction='in'/><arg type='a(sa{sv})' direction='in'/><arg type='s' direction='in'/><arg type='a{sv}' direction='in'/><arg type='o' direction='out'/></method>
                    <signal name='Activated'><arg type='o'/><arg type='s'/><arg type='t'/><arg type='a{sv}'/></signal>
                </interface>
                <interface name='org.freedesktop.portal.Request'><method name='Close'/><signal name='Response'><arg type='u'/><arg type='a{sv}'/></signal></interface>
                <interface name='org.freedesktop.portal.Session'><method name='Close'/><signal name='Closed'/></interface>
                <interface name='org.freedesktop.host.portal.Registry'><method name='Register'><arg type='s' direction='in'/><arg type='a{sv}' direction='in'/></method></interface>
                </node>)", nullptr);
            std::promise<void> ready;
            auto future = ready.get_future();
            worker = std::thread([&]()
            {
                g_main_context_push_thread_default(context);
                connection = g_dbus_connection_new_for_address_sync(g_getenv("DBUS_SESSION_BUS_ADDRESS"),
                    GDBusConnectionFlags(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION), nullptr, nullptr, nullptr);
                TEST_ASSERT(connection);
                Export(PortalPath, 0);
                Export(PortalPath, 3);
                auto reply = g_dbus_connection_call_sync(connection, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                    "org.freedesktop.DBus", "RequestName", g_variant_new("(su)", PortalName, 0), nullptr, G_DBUS_CALL_FLAGS_NONE, 2000, nullptr, nullptr);
                TEST_ASSERT(reply);
                g_variant_unref(reply);
                ready.set_value();
                g_main_loop_run(loop);
                for (auto object : objects) g_dbus_connection_unregister_object(connection, object);
                reply = g_dbus_connection_call_sync(connection, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                    "org.freedesktop.DBus", "ReleaseName", g_variant_new("(s)", PortalName), nullptr, G_DBUS_CALL_FLAGS_NONE, 2000, nullptr, nullptr);
                TEST_ASSERT(reply);
                g_variant_unref(reply);
                g_dbus_connection_close_sync(connection, nullptr, nullptr);
                g_object_unref(connection);
                g_main_context_pop_thread_default(context);
            });
            future.get();
        }

        ~Portal()
        {
            g_main_loop_quit(loop);
            worker.join();
            g_main_loop_unref(loop);
            g_main_context_unref(context);
            g_dbus_node_info_unref(schema);
        }

        std::string Trigger(size_t index)
        {
            std::lock_guard lock(mutex);
            return sessions.at(index).shortcut;
        }

        void Activate(size_t index, bool wrongId = false, bool wrongSender = false)
        {
            std::lock_guard lock(mutex);
            auto& session = sessions.at(index);
            GVariantBuilder options;
            g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
            auto source = wrongSender ? g_dbus_connection_new_for_address_sync(g_getenv("DBUS_SESSION_BUS_ADDRESS"),
                GDBusConnectionFlags(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION), nullptr, nullptr, nullptr) : connection;
            g_dbus_connection_emit_signal(source, session.sender.c_str(), PortalPath, ShortcutInterface, "Activated",
                g_variant_new("(osta{sv})", session.path.c_str(), wrongId ? "wrong" : session.shortcut.c_str(), guint64(0), &options), nullptr);
            if (wrongSender)
            {
                g_dbus_connection_flush_sync(source, nullptr, nullptr);
                g_dbus_connection_close_sync(source, nullptr, nullptr);
                g_object_unref(source);
            }
        }
    };

    void PumpUntil(WGacInputService& service, const Func<bool()>& done)
    {
        auto deadline = g_get_monotonic_time() + 3000000;
        do
        {
            service.PumpEvents();
            if (done()) return;
            g_usleep(1000);
        } while (g_get_monotonic_time() < deadline);
        TEST_ASSERT(done());
    }

    void Pump(WGacInputService& service)
    {
        auto deadline = g_get_monotonic_time() + 50000;
        PumpUntil(service, [&]() { return g_get_monotonic_time() >= deadline; });
    }
}

TEST_FILE
{
    TEST_CASE(L"Portal activation preserves IDs, modifiers, UI thread and registration lifetime")
    {
        Portal portal;
        std::vector<vint> events;
        auto owner = std::this_thread::get_id();
        WGacInputService service(nullptr, [&](vint id)
        {
            TEST_ASSERT(std::this_thread::get_id() == owner);
            events.push_back(id);
        });
        auto q = service.RegisterGlobalShortcutKey(true, true, true, true, VKEY::KEY_Q);
        auto f8 = service.RegisterGlobalShortcutKey(true, true, true, true, VKEY::KEY_F8);
        TEST_ASSERT(q >= 0 && f8 > q);
        TEST_ASSERT(service.RegisterGlobalShortcutKey(true, true, true, true, VKEY::KEY_Q) == (vint)NativeGlobalShortcutKeyResult::Occupied);
        TEST_ASSERT(service.RegisterGlobalShortcutKey(false, false, false, false, VKEY::KEY_UNKNOWN) == (vint)NativeGlobalShortcutKeyResult::NotSupported);
        PumpUntil(service, [&]() { return portal.bound == 2; });
        Pump(service);
        TEST_ASSERT(portal.Trigger(0) == "CTRL+SHIFT+ALT+LOGO+q");
        TEST_ASSERT(portal.Trigger(1) == "CTRL+SHIFT+ALT+LOGO+F8");
        portal.Activate(0, true);
        portal.Activate(0, false, true);
        Pump(service);
        TEST_ASSERT(events.empty());
        portal.Activate(0);
        portal.Activate(1);
        PumpUntil(service, [&]() { return events.size() == 2; });
        TEST_ASSERT(events[0] == q && events[1] == f8);
        TEST_ASSERT(!service.UnregisterGlobalShortcutKey(-1));
        TEST_ASSERT(!service.UnregisterGlobalShortcutKey(123456));
        TEST_ASSERT(service.UnregisterGlobalShortcutKey(q));
        TEST_ASSERT(!service.UnregisterGlobalShortcutKey(q));
        PumpUntil(service, [&]() { return portal.closed == 1; });
        portal.Activate(0);
        Pump(service);
        TEST_ASSERT(events.size() == 2);
        auto again = service.RegisterGlobalShortcutKey(true, true, true, true, VKEY::KEY_Q);
        TEST_ASSERT(again > f8);
        PumpUntil(service, [&]() { return portal.bound == 3; });
        Pump(service);
        portal.Activate(2);
        PumpUntil(service, [&]() { return events.size() == 3; });
        TEST_ASSERT(events.back() == again);
    });

    TEST_CASE(L"Portal rejection and cancellation never activate a shortcut")
    {
        Portal portal;
        portal.denied = true;
        vint events = 0;
        WGacInputService service(nullptr, [&](vint) { events++; });
        auto denied = service.RegisterGlobalShortcutKey(true, false, false, false, VKEY::KEY_Q);
        PumpUntil(service, [&]() { return portal.closed == 1; });
        portal.Activate(0);
        Pump(service);
        TEST_ASSERT(events == 0);
        TEST_ASSERT(service.UnregisterGlobalShortcutKey(denied));
        portal.denied = false;
        portal.pending = true;
        auto pending = service.RegisterGlobalShortcutKey(true, false, false, false, VKEY::KEY_Q);
        PumpUntil(service, [&]() { return portal.bound == 2; });
        TEST_ASSERT(service.UnregisterGlobalShortcutKey(pending));
        PumpUntil(service, [&]() { return portal.cancelled == 1 && portal.closed == 2; });
        portal.Activate(1);
        Pump(service);
        TEST_ASSERT(events == 0);
    });

    TEST_CASE(L"Unavailable portal reports unsupported instead of a successful stub ID")
    {
        WGacInputService service(nullptr, [](vint) {});
        TEST_ASSERT(service.RegisterGlobalShortcutKey(true, false, false, false, VKEY::KEY_Q) == (vint)NativeGlobalShortcutKeyResult::NotSupported);
        TEST_ASSERT(!service.UnregisterGlobalShortcutKey(1));
    });
}

int main(int argc, char* argv[])
{
    auto bus = g_test_dbus_new(G_TEST_DBUS_NONE);
    g_test_dbus_up(bus);
    g_setenv("WGAC_APPLICATION_ID", "org.gaclib.wGac.UnitTest", true);
    auto result = unittest::UnitTest::RunAndDisposeTests(argc, argv);
    g_test_dbus_down(bus);
    g_object_unref(bus);
    FinalizeGlobalStorage();
    return result;
}
