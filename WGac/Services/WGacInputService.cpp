#include "WGacInputService.h"
#include <gio/gio.h>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace vl {
namespace presentation {
namespace wayland {

namespace
{
    constexpr const char* PortalName = "org.freedesktop.portal.Desktop";
    constexpr const char* PortalPath = "/org/freedesktop/portal/desktop";
    constexpr const char* ShortcutInterface = "org.freedesktop.portal.GlobalShortcuts";
    constexpr const char* RequestInterface = "org.freedesktop.portal.Request";
    constexpr const char* SessionInterface = "org.freedesktop.portal.Session";

    std::string ShortcutTrigger(bool ctrl, bool shift, bool alt, bool osSuper, VKEY code)
    {
        std::string key;
        auto value = (vint)code;
        if (0x41 <= value && value <= 0x5A) key = char('a' + value - 0x41);
        else if (0x30 <= value && value <= 0x39) key = char(value);
        else if (0x70 <= value && value <= 0x87) key = "F" + std::to_string(value - 0x6F);
        else if (0x60 <= value && value <= 0x69) key = "KP_" + std::to_string(value - 0x60);
        else switch (value)
        {
        case 0x08: key = "BackSpace"; break;
        case 0x09: key = "Tab"; break;
        case 0x0D: key = "Return"; break;
        case 0x13: key = "Pause"; break;
        case 0x14: key = "Caps_Lock"; break;
        case 0x1B: key = "Escape"; break;
        case 0x20: key = "space"; break;
        case 0x21: key = "Prior"; break;
        case 0x22: key = "Next"; break;
        case 0x23: key = "End"; break;
        case 0x24: key = "Home"; break;
        case 0x25: key = "Left"; break;
        case 0x26: key = "Up"; break;
        case 0x27: key = "Right"; break;
        case 0x28: key = "Down"; break;
        case 0x2C: key = "Print"; break;
        case 0x2D: key = "Insert"; break;
        case 0x2E: key = "Delete"; break;
        case 0x6A: key = "KP_Multiply"; break;
        case 0x6B: key = "KP_Add"; break;
        case 0x6D: key = "KP_Subtract"; break;
        case 0x6E: key = "KP_Decimal"; break;
        case 0x6F: key = "KP_Divide"; break;
        case 0xBA: key = "semicolon"; break;
        case 0xBB: key = "equal"; break;
        case 0xBC: key = "comma"; break;
        case 0xBD: key = "minus"; break;
        case 0xBE: key = "period"; break;
        case 0xBF: key = "slash"; break;
        case 0xC0: key = "grave"; break;
        case 0xDB: key = "bracketleft"; break;
        case 0xDC: key = "backslash"; break;
        case 0xDD: key = "bracketright"; break;
        case 0xDE: key = "apostrophe"; break;
        default: return {};
        }
        return std::string(ctrl ? "CTRL+" : "") + (shift ? "SHIFT+" : "")
            + (alt ? "ALT+" : "") + (osSuper ? "LOGO+" : "") + key;
    }
}

class WGacGlobalShortcutService
{
    struct Shortcut
    {
        vint id;
        std::string trigger;
        std::string session;
        std::string request;
        bool creating = true;
        bool bound = false;
    };

    Func<void(vint)> activated;
    GMainContext* context = g_main_context_new();
    GDBusConnection* connection = nullptr;
    std::string portalOwner;
    std::vector<guint> subscriptions;
    std::vector<Shortcut> shortcuts;

    bool Connect()
    {
        if (connection) return !portalOwner.empty() && !g_dbus_connection_is_closed(connection);
        if (!activated) return false;

        auto address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
        if (!address) return false;
        connection = g_dbus_connection_new_for_address_sync(address,
            GDBusConnectionFlags(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
            nullptr, nullptr, nullptr);
        g_free(address);
        if (!connection) return false;
        g_dbus_connection_set_exit_on_close(connection, false);

        // Command-line launches have no desktop/sandbox identity. Register the
        // caller's installed desktop ID before making any other portal call.
        auto applicationId = g_getenv("WGAC_APPLICATION_ID");
        if (applicationId && *applicationId)
        {
            GVariantBuilder options;
            g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
            GError* error = nullptr;
            auto reply = g_dbus_connection_call_sync(connection, PortalName, PortalPath,
                "org.freedesktop.host.portal.Registry", "Register", g_variant_new("(sa{sv})", applicationId, &options),
                nullptr, G_DBUS_CALL_FLAGS_NONE, 3000, nullptr, &error);
            if (reply) g_variant_unref(reply);
            else
            {
                g_warning("wGac portal application registration: %s", error->message);
                g_error_free(error);
                return false;
            }
        }

        // Resolve the owner and check the interface instead of guessing from the OS version.
        auto proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
            nullptr, PortalName, PortalPath, ShortcutInterface, nullptr, nullptr);
        if (!proxy) return false;
        auto version = g_dbus_proxy_get_cached_property(proxy, "version");
        auto owner = g_dbus_proxy_get_name_owner(proxy);
        if (version && g_variant_is_of_type(version, G_VARIANT_TYPE_UINT32)
            && g_variant_get_uint32(version) >= 1 && owner)
        {
            portalOwner = owner;
        }
        if (version) g_variant_unref(version);
        g_free(owner);
        g_object_unref(proxy);
        if (portalOwner.empty()) return false;

        // Both native and TUI controllers pump this context on their UI thread.
        g_main_context_push_thread_default(context);
        for (auto interface : {RequestInterface, ShortcutInterface, SessionInterface})
        {
            subscriptions.push_back(g_dbus_connection_signal_subscribe(connection, portalOwner.c_str(),
                interface, nullptr, nullptr, nullptr, G_DBUS_SIGNAL_FLAGS_NONE, Signal, this, nullptr));
        }
        subscriptions.push_back(g_dbus_connection_signal_subscribe(connection, "org.freedesktop.DBus",
            "org.freedesktop.DBus", "NameOwnerChanged", "/org/freedesktop/DBus", PortalName,
            G_DBUS_SIGNAL_FLAGS_NONE, Signal, this, nullptr));
        g_main_context_pop_thread_default(context);
        return true;
    }

    std::string Token()
    {
        auto uuid = g_uuid_string_random();
        std::string token = "wgac_" + std::string(uuid);
        g_free(uuid);
        std::replace(token.begin(), token.end(), '-', '_');
        return token;
    }

    std::string Handle(const char* kind, const std::string& token)
    {
        std::string sender = g_dbus_connection_get_unique_name(connection) + 1;
        std::replace(sender.begin(), sender.end(), '.', '_');
        return std::string(PortalPath) + "/" + kind + "/" + sender + "/" + token;
    }

    bool Request(Shortcut& shortcut, const char* method, GVariant* arguments)
    {
        // The method returns a request handle immediately. User approval arrives later
        // through Response, without blocking the application or nesting its event loop.
        GError* error = nullptr;
        auto reply = g_dbus_connection_call_sync(connection, portalOwner.c_str(), PortalPath,
            ShortcutInterface, method, arguments, G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE,
            3000, nullptr, &error);
        if (!reply)
        {
            g_warning("wGac global shortcut %s: %s", method, error->message);
            g_error_free(error);
            Close(shortcut);
            return false;
        }
        const char* path;
        g_variant_get(reply, "(&o)", &path);
        shortcut.request = path;
        g_variant_unref(reply);
        return true;
    }

    void Close(Shortcut& shortcut)
    {
        shortcut.bound = false;
        if (portalOwner.empty()) return;
        for (auto item : {std::make_pair(shortcut.request, RequestInterface), std::make_pair(shortcut.session, SessionInterface)})
        {
            if (!item.first.empty()) g_dbus_connection_call(connection, portalOwner.c_str(), item.first.c_str(),
                item.second, "Close", nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, 1000, nullptr, nullptr, nullptr);
        }
        shortcut.request.clear();
        shortcut.session.clear();
    }

    static void Signal(GDBusConnection*, const gchar*, const gchar* path, const gchar* interface,
        const gchar* signal, GVariant* parameters, gpointer data)
    {
        static_cast<WGacGlobalShortcutService*>(data)->OnSignal(path, interface, signal, parameters);
    }

    void OnSignal(const char* path, const char* interface, const char* signal, GVariant* parameters)
    {
        if (std::string(signal) == "NameOwnerChanged")
        {
            portalOwner.clear();
            for (auto& shortcut : shortcuts) shortcut.bound = false;
            return;
        }
        if (std::string(interface) == ShortcutInterface && std::string(signal) == "Activated")
        {
            const char* session;
            const char* id;
            guint64 timestamp;
            GVariant* options;
            g_variant_get(parameters, "(&o&st@a{sv})", &session, &id, &timestamp, &options);
            g_variant_unref(options);
            for (const auto& shortcut : shortcuts)
            {
                if (shortcut.bound && shortcut.session == session && shortcut.trigger == id)
                {
                    auto registeredId = shortcut.id;
                    activated(registeredId);
                    return; // The callback may unregister this shortcut.
                }
            }
            return;
        }
        for (auto& shortcut : shortcuts)
        {
            if (std::string(interface) == SessionInterface && std::string(signal) == "Closed" && shortcut.session == path)
            {
                shortcut.bound = false;
                shortcut.session.clear();
                return;
            }
            if (std::string(interface) != RequestInterface || std::string(signal) != "Response" || shortcut.request != path) continue;
            guint response;
            GVariant* results;
            g_variant_get(parameters, "(u@a{sv})", &response, &results);
            shortcut.request.clear();
            if (response != 0)
            {
                g_message("wGac global shortcut %s was not approved (portal response %u).", shortcut.trigger.c_str(), response);
                Close(shortcut);
            }
            else if (shortcut.creating)
            {
                const char* session = nullptr;
                if (g_variant_lookup(results, "session_handle", "&s", &session) && g_variant_is_object_path(session))
                {
                    shortcut.session = session;
                    shortcut.creating = false;
                    GVariantBuilder properties;
                    g_variant_builder_init(&properties, G_VARIANT_TYPE_VARDICT);
                    auto description = "GacUI global shortcut: " + shortcut.trigger;
                    g_variant_builder_add(&properties, "{sv}", "description", g_variant_new_string(description.c_str()));
                    g_variant_builder_add(&properties, "{sv}", "preferred_trigger", g_variant_new_string(shortcut.trigger.c_str()));
                    GVariantBuilder bindings;
                    g_variant_builder_init(&bindings, G_VARIANT_TYPE("a(sa{sv})"));
                    g_variant_builder_add(&bindings, "(sa{sv})", shortcut.trigger.c_str(), &properties);
                    GVariantBuilder options;
                    g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
                    g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(Token().c_str()));
                    Request(shortcut, "BindShortcuts", g_variant_new("(oa(sa{sv})sa{sv})", shortcut.session.c_str(), &bindings, "", &options));
                }
                else Close(shortcut);
            }
            else
            {
                auto bindings = g_variant_lookup_value(results, "shortcuts", G_VARIANT_TYPE("a(sa{sv})"));
                if (bindings)
                {
                    GVariantIter iterator;
                    g_variant_iter_init(&iterator, bindings);
                    const char* id;
                    GVariant* properties;
                    while (g_variant_iter_next(&iterator, "(&s@a{sv})", &id, &properties))
                    {
                        if (shortcut.trigger == id) shortcut.bound = true;
                        g_variant_unref(properties);
                    }
                    g_variant_unref(bindings);
                }
                if (!shortcut.bound) Close(shortcut);
            }
            g_variant_unref(results);
            return;
        }
    }

public:
    WGacGlobalShortcutService(const Func<void(vint)>& callback) : activated(callback) {}

    ~WGacGlobalShortcutService()
    {
        if (connection)
        {
            for (auto subscription : subscriptions) g_dbus_connection_signal_unsubscribe(connection, subscription);
            for (auto& shortcut : shortcuts) Close(shortcut);
            // This connection belongs only to shortcuts; disconnecting also releases
            // sessions whose CreateSession response was still in flight at shutdown.
            g_dbus_connection_flush_sync(connection, nullptr, nullptr);
            g_dbus_connection_close_sync(connection, nullptr, nullptr);
            g_object_unref(connection);
        }
        g_main_context_unref(context);
    }

    vint Register(vint id, const std::string& trigger)
    {
        if (trigger.empty() || !Connect()) return (vint)NativeGlobalShortcutKeyResult::NotSupported;
        for (const auto& shortcut : shortcuts)
        {
            if (shortcut.trigger == trigger) return (vint)NativeGlobalShortcutKeyResult::Occupied;
        }
        Shortcut shortcut{id, trigger};
        auto token = Token();
        shortcut.session = Handle("session", token);
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(Token().c_str()));
        g_variant_builder_add(&options, "{sv}", "session_handle_token", g_variant_new_string(token.c_str()));
        if (!Request(shortcut, "CreateSession", g_variant_new("(a{sv})", &options)))
            return (vint)NativeGlobalShortcutKeyResult::NotSupported;
        shortcuts.push_back(std::move(shortcut));
        return id;
    }

    bool Unregister(vint id)
    {
        auto found = std::find_if(shortcuts.begin(), shortcuts.end(), [=](const Shortcut& shortcut) { return shortcut.id == id; });
        if (found == shortcuts.end()) return false;
        Close(*found);
        shortcuts.erase(found);
        return true;
    }

    void PumpEvents()
    {
        while (g_main_context_iteration(context, false)) {}
    }
};

WGacInputService::WGacInputService(TimerFunc _timer, const Func<void(vint)>& shortcutActivated)
    : timer(_timer)
    , isTimerEnabled(false)
    , usedHotKeys(0)
    , keyNames(256)
    , globalShortcuts(std::make_unique<WGacGlobalShortcutService>(shortcutActivated))
{
    InitKeyMapping();
}

WGacInputService::~WGacInputService() = default;

void WGacInputService::PumpEvents()
{
    globalShortcuts->PumpEvents();
}

WString WGacInputService::GetKeyNameInternal(VKEY code)
{
    if ((vint)code < 8) return L"?";

    // Map common keys
    switch ((vint)code) {
        case 0x08: return L"Backspace";
        case 0x09: return L"Tab";
        case 0x0D: return L"Enter";
        case 0x10: return L"Shift";
        case 0x11: return L"Ctrl";
        case 0x12: return L"Alt";
        case 0x14: return L"CapsLock";
        case 0x1B: return L"Escape";
        case 0x20: return L"Space";
        case 0x21: return L"PageUp";
        case 0x22: return L"PageDown";
        case 0x23: return L"End";
        case 0x24: return L"Home";
        case 0x25: return L"Left";
        case 0x26: return L"Up";
        case 0x27: return L"Right";
        case 0x28: return L"Down";
        case 0x2D: return L"Insert";
        case 0x2E: return L"Delete";
        case 0x5B: return L"Super";
        case 0x5C: return L"Super";
        case (vint)VKEY::KEY_LEFT_BRACKET: return L"[";
        case (vint)VKEY::KEY_RIGHT_BRACKET: return L"]";
        default: break;
    }

    // Letters A-Z
    if (code >= VKEY::KEY_A && code <= VKEY::KEY_Z) {
        wchar_t buf[2] = { static_cast<wchar_t>(L'A' + ((vint)code - (vint)VKEY::KEY_A)), 0 };
        return WString(buf);
    }

    // Numbers 0-9
    if (code >= VKEY::KEY_0 && code <= VKEY::KEY_9) {
        wchar_t buf[2] = { static_cast<wchar_t>(L'0' + ((vint)code - (vint)VKEY::KEY_0)), 0 };
        return WString(buf);
    }

    // Function keys F1-F12
    if (code >= VKEY::KEY_F1 && code <= VKEY::KEY_F12) {
        return L"F" + itow((vint)code - (vint)VKEY::KEY_F1 + 1);
    }

    return L"?";
}

void WGacInputService::InitKeyMapping()
{
    for (vint i = 0; i < keyNames.Count(); i++)
    {
        keyNames[i] = GetKeyNameInternal((VKEY)i);
        if (keyNames[i] != L"?")
        {
            keys.Set(keyNames[i], (VKEY)i);
        }
    }
}

void WGacInputService::StartTimer()
{
    isTimerEnabled = true;
}

void WGacInputService::StopTimer()
{
    isTimerEnabled = false;
}

bool WGacInputService::IsTimerEnabled()
{
    return isTimerEnabled;
}

bool WGacInputService::IsKeyPressing(VKEY code)
{
    return false;
}

bool WGacInputService::IsKeyToggled(VKEY code)
{
    return false;
}

WString WGacInputService::GetKeyName(VKEY code)
{
    if (0 <= (vint)code && (vint)code < keyNames.Count())
    {
        return keyNames[(vint)code];
    }
    else
    {
        return L"?";
    }
}

VKEY WGacInputService::GetKey(const WString& name)
{
    vint index = keys.Keys().IndexOf(name);
    return index == -1 ? VKEY::KEY_UNKNOWN : keys.Values()[index];
}

vint WGacInputService::RegisterGlobalShortcutKey(bool ctrl, bool shift, bool alt, bool osSuper, VKEY code)
{
    return globalShortcuts->Register(++usedHotKeys, ShortcutTrigger(ctrl, shift, alt, osSuper, code));
}

bool WGacInputService::UnregisterGlobalShortcutKey(vint id)
{
    return globalShortcuts->Unregister(id);
}

}
}
}
