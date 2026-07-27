#include "WGacDialogService.h"
#include "../WGacController.h"

#include <gio/gio.h>

#include <atomic>
#include <cstring>
#include <string>
#include <vector>

namespace vl {
namespace presentation {
namespace wayland {

namespace {

constexpr const char* PortalBusName = "org.freedesktop.portal.Desktop";
constexpr const char* PortalObjectPath = "/org/freedesktop/portal/desktop";
constexpr const char* PortalFileChooserInterface = "org.freedesktop.portal.FileChooser";
constexpr const char* PortalRequestInterface = "org.freedesktop.portal.Request";
constexpr const char* DbusBusName = "org.freedesktop.DBus";
constexpr const char* DbusObjectPath = "/org/freedesktop/DBus";
constexpr const char* DbusInterface = "org.freedesktop.DBus";
constexpr gint PortalActivationTimeout = 5000;
constexpr gint PortalMethodTimeout = 10000;
constexpr gint PortalCloseTimeout = 1000;

std::atomic_flag fileDialogActive = ATOMIC_FLAG_INIT;

struct PortalFilter
{
    std::string name;
    std::vector<std::string> patterns;
};

struct PendingPortalResponse
{
    std::string path;
    guint response = 2;
    GVariant* results = nullptr;
};

struct PortalResponse
{
    std::string portalOwner;
    std::string requestPath;
    std::vector<PendingPortalResponse> pending;
    guint response = 2;
    GVariant* results = nullptr;
    bool requestFinalized = false;
    bool requestCreated = false;
    bool callStarted = false;
    bool callCompleted = false;
    bool received = false;
    bool ownerLost = false;
    bool failed = false;

    ~PortalResponse()
    {
        if (results)
        {
            g_variant_unref(results);
        }
        for (auto&& item : pending)
        {
            if (item.results)
            {
                g_variant_unref(item.results);
            }
        }
    }

    void Accept(guint value, GVariant* values)
    {
        if (received)
        {
            g_variant_unref(values);
            return;
        }

        response = value;
        results = values;
        received = true;
    }

    void FinalizeRequest(const gchar* path)
    {
        requestPath = path;
        requestFinalized = true;
        requestCreated = true;

        for (auto&& item : pending)
        {
            if (!received && item.path == requestPath)
            {
                auto values = item.results;
                item.results = nullptr;
                Accept(item.response, values);
            }
            else if (item.results)
            {
                g_variant_unref(item.results);
                item.results = nullptr;
            }
        }
        pending.clear();
    }

    GVariant* TakeResults()
    {
        auto value = results;
        results = nullptr;
        return value;
    }
};

class ScopedFileDialog
{
public:
    ScopedFileDialog()
        : acquired(!fileDialogActive.test_and_set(std::memory_order_acquire))
    {
    }

    ~ScopedFileDialog()
    {
        if (acquired)
        {
            fileDialogActive.clear(std::memory_order_release);
        }
    }

    bool IsAcquired() const
    {
        return acquired;
    }

private:
    bool acquired = false;
};

std::string ToUtf8(const WString& text)
{
    auto utf8 = wtou8(text);
    return std::string(reinterpret_cast<const char*>(utf8.Buffer()), static_cast<size_t>(utf8.Length()));
}

WString FromUtf8(const char* text, size_t length)
{
    return u8tow(U8String::CopyFrom(
        reinterpret_cast<const char8_t*>(text),
        static_cast<vint>(length)
        ));
}

bool ToFileSystemPath(const WString& path, std::string& result)
{
    auto utf8 = ToUtf8(path);
    GError* error = nullptr;
    gsize bytesWritten = 0;
    gchar* converted = g_filename_from_utf8(
        utf8.data(),
        static_cast<gssize>(utf8.size()),
        nullptr,
        &bytesWritten,
        &error
        );
    if (!converted)
    {
        if (error)
        {
            g_error_free(error);
        }
        return false;
    }

    result.assign(converted, bytesWritten);
    g_free(converted);
    return true;
}

std::string CreateHandleToken()
{
    gchar* uuid = g_uuid_string_random();
    std::string token = "wgac_";
    for (const gchar* reading = uuid; *reading; reading++)
    {
        token += *reading == '-' ? '_' : *reading;
    }
    g_free(uuid);
    return token;
}

std::vector<WString> Split(const WString& text, wchar_t delimiter)
{
    std::vector<WString> result;
    vint begin = 0;
    for (vint i = 0; i <= text.Length(); i++)
    {
        if (i == text.Length() || text[i] == delimiter)
        {
            result.push_back(text.Sub(begin, i - begin));
            begin = i + 1;
        }
    }
    return result;
}

std::vector<PortalFilter> ParseFilters(const WString& filter)
{
    std::vector<PortalFilter> result;
    auto fields = Split(filter, L'|');
    for (size_t i = 0; i + 1 < fields.size(); i += 2)
    {
        PortalFilter parsed;
        parsed.name = ToUtf8(fields[i]);
        for (const auto& pattern : Split(fields[i + 1], L';'))
        {
            auto patternUtf8 = ToUtf8(pattern);
            if (patternUtf8 == "*.*")
            {
                patternUtf8 = "*";
            }
            if (!patternUtf8.empty())
            {
                parsed.patterns.push_back(std::move(patternUtf8));
            }
        }
        if (!parsed.name.empty() && !parsed.patterns.empty())
        {
            result.push_back(std::move(parsed));
        }
    }
    return result;
}

GVariant* CreatePortalFilter(const PortalFilter& filter)
{
    GVariantBuilder patterns;
    g_variant_builder_init(&patterns, G_VARIANT_TYPE("a(us)"));
    for (const auto& pattern : filter.patterns)
    {
        g_variant_builder_add(&patterns, "(us)", static_cast<guint32>(0), pattern.c_str());
    }
    return g_variant_new("(s@a(us))", filter.name.c_str(), g_variant_builder_end(&patterns));
}

void AddPathOption(GVariantBuilder& options, const char* name, const std::string& path)
{
    auto bytes = g_variant_new_fixed_array(
        G_VARIANT_TYPE_BYTE,
        path.c_str(),
        path.size() + 1,
        sizeof(guint8)
        );
    g_variant_builder_add(&options, "{sv}", name, bytes);
}

bool AddCurrentFolder(GVariantBuilder& options, const WString& initialDirectory)
{
    if (initialDirectory == WString::Empty)
    {
        return true;
    }

    std::string directory;
    if (!ToFileSystemPath(initialDirectory, directory))
    {
        return false;
    }
    if (!g_path_is_absolute(directory.c_str()) || !g_file_test(directory.c_str(), G_FILE_TEST_IS_DIR))
    {
        return true;
    }

    AddPathOption(options, "current_folder", directory);
    return true;
}

WString GetFileName(const WString& path)
{
    vint separator = -1;
    for (vint i = 0; i < path.Length(); i++)
    {
        if (path[i] == L'/' || path[i] == L'\\')
        {
            separator = i;
        }
    }
    return path.Right(path.Length() - separator - 1);
}

WString ApplyDefaultExtension(const WString& path, const WString& defaultExtension)
{
    if (path == WString::Empty || defaultExtension == WString::Empty)
    {
        return path;
    }

    auto fileName = GetFileName(path);
    for (vint i = 0; i < fileName.Length(); i++)
    {
        if (fileName[i] == L'.')
        {
            return path;
        }
    }

    return defaultExtension[0] == L'.'
        ? path + defaultExtension
        : path + L"." + defaultExtension;
}

WString GetEffectiveDefaultExtension(
    const std::vector<PortalFilter>& filters,
    vint selectionFilterIndex,
    const WString& defaultExtension
    )
{
    size_t filterIndex = 0;
    bool hasFilter = !filters.empty();
    if (selectionFilterIndex >= 0 && static_cast<size_t>(selectionFilterIndex) < filters.size())
    {
        filterIndex = static_cast<size_t>(selectionFilterIndex);
    }
    if (hasFilter)
    {
        for (const auto& pattern : filters[filterIndex].patterns)
        {
            if (pattern.size() < 3 || pattern[0] != '*' || pattern[1] != '.')
            {
                continue;
            }

            auto extension = pattern.substr(2);
            if (extension.find_first_of("*?;[]") == std::string::npos)
            {
                return FromUtf8(extension.data(), extension.size());
            }
        }
    }
    return defaultExtension;
}

bool AddInitialFileOptions(
    GVariantBuilder& options,
    bool save,
    const WString& initialFileName,
    const WString& initialDirectory,
    const WString& effectiveDefaultExtension
    )
{
    if (initialFileName == WString::Empty)
    {
        return AddCurrentFolder(options, initialDirectory);
    }

    std::string initialPath;
    if (!ToFileSystemPath(initialFileName, initialPath))
    {
        return false;
    }

    if (g_path_is_absolute(initialPath.c_str()))
    {
        if (save)
        {
            if (g_file_test(initialPath.c_str(), G_FILE_TEST_IS_DIR))
            {
                AddPathOption(options, "current_folder", initialPath);
                return true;
            }

            if (g_file_test(initialPath.c_str(), G_FILE_TEST_EXISTS))
            {
                AddPathOption(options, "current_file", initialPath);
                return true;
            }

            auto effectiveFileName = ApplyDefaultExtension(initialFileName, effectiveDefaultExtension);
            if (!ToFileSystemPath(effectiveFileName, initialPath))
            {
                return false;
            }

            gchar* parent = g_path_get_dirname(initialPath.c_str());
            if (parent)
            {
                if (g_path_is_absolute(parent) && g_file_test(parent, G_FILE_TEST_IS_DIR))
                {
                    AddPathOption(options, "current_folder", parent);
                }
                g_free(parent);
            }

            auto fileName = ToUtf8(GetFileName(effectiveFileName));
            if (!fileName.empty())
            {
                g_variant_builder_add(&options, "{sv}", "current_name", g_variant_new_string(fileName.c_str()));
            }
            return true;
        }

        gchar* parent = g_path_get_dirname(initialPath.c_str());
        if (parent)
        {
            if (g_path_is_absolute(parent) && g_file_test(parent, G_FILE_TEST_IS_DIR))
            {
                AddPathOption(options, "current_folder", parent);
            }
            g_free(parent);
        }
        return true;
    }

    if (!AddCurrentFolder(options, initialDirectory))
    {
        return false;
    }

    if (save)
    {
        auto effectiveFileName = ApplyDefaultExtension(
            GetFileName(initialFileName),
            effectiveDefaultExtension
            );
        auto fileName = ToUtf8(effectiveFileName);
        if (!fileName.empty())
        {
            g_variant_builder_add(&options, "{sv}", "current_name", g_variant_new_string(fileName.c_str()));
        }
    }
    return true;
}

void PortalResponseReceived(
    GDBusConnection*,
    const gchar* senderName,
    const gchar* objectPath,
    const gchar*,
    const gchar*,
    GVariant* parameters,
    gpointer userData
    )
{
    auto response = static_cast<PortalResponse*>(userData);
    if (!senderName ||
        !objectPath ||
        response->portalOwner != senderName)
    {
        return;
    }
    if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(ua{sv})")))
    {
        if (response->requestFinalized && response->requestPath == objectPath)
        {
            response->failed = true;
        }
        return;
    }

    guint result = 2;
    GVariant* values = nullptr;
    g_variant_get(parameters, "(u@a{sv})", &result, &values);

    try
    {
        if (response->requestFinalized)
        {
            if (response->requestPath == objectPath)
            {
                response->Accept(result, values);
            }
            else
            {
                g_variant_unref(values);
            }
        }
        else
        {
            for (const auto& item : response->pending)
            {
                if (item.path == objectPath)
                {
                    g_variant_unref(values);
                    return;
                }
            }
            response->pending.push_back({objectPath, result, values});
        }
    }
    catch (...)
    {
        g_variant_unref(values);
        response->failed = true;
    }
}

void PortalOwnerChanged(
    GDBusConnection*,
    const gchar*,
    const gchar*,
    const gchar*,
    const gchar*,
    GVariant* parameters,
    gpointer userData
    )
{
    auto response = static_cast<PortalResponse*>(userData);
    if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(sss)")))
    {
        return;
    }

    const gchar* name = nullptr;
    const gchar* oldOwner = nullptr;
    const gchar* newOwner = nullptr;
    g_variant_get(parameters, "(&s&s&s)", &name, &oldOwner, &newOwner);
    if (std::strcmp(name, PortalBusName) == 0 &&
        response->portalOwner == oldOwner &&
        response->portalOwner != newOwner)
    {
        response->ownerLost = true;
    }
}

void PortalMethodReturned(GObject* source, GAsyncResult* result, gpointer userData)
{
    auto response = static_cast<PortalResponse*>(userData);
    GError* error = nullptr;
    GVariant* reply = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), result, &error);

    try
    {
        if (!reply || !g_variant_is_of_type(reply, G_VARIANT_TYPE("(o)")))
        {
            response->failed = true;
        }
        else
        {
            const gchar* actualRequestPath = nullptr;
            g_variant_get(reply, "(&o)", &actualRequestPath);
            if (actualRequestPath)
            {
                response->FinalizeRequest(actualRequestPath);
            }
            else
            {
                response->failed = true;
            }
        }
    }
    catch (...)
    {
        response->failed = true;
    }

    if (reply)
    {
        g_variant_unref(reply);
    }
    if (error)
    {
        g_error_free(error);
    }
    response->callCompleted = true;
}

void DispatchPortalContext(GMainContext* context)
{
    g_main_context_push_thread_default(context);
    while (g_main_context_iteration(context, FALSE))
    {
    }
    g_main_context_pop_thread_default(context);
}

bool RunWgacModalCycle()
{
    try
    {
        auto controller = GetWGacController();
        auto windowService = controller ? controller->WindowService() : nullptr;
        return windowService && windowService->RunOneCycle();
    }
    catch (...)
    {
        return false;
    }
}

bool RunPortalFileChooser(
    const char* method,
    const std::string& title,
    GVariant* options,
    guint& responseCode,
    GVariant*& responseResults
    )
{
    responseCode = 2;
    responseResults = nullptr;
    options = g_variant_ref_sink(options);

    GMainContext* context = g_main_context_new();
    g_main_context_push_thread_default(context);

    GError* error = nullptr;
    GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (!connection)
    {
        if (error)
        {
            g_error_free(error);
        }
        g_main_context_pop_thread_default(context);
        g_main_context_unref(context);
        g_variant_unref(options);
        return false;
    }

    PortalResponse response;
    auto token = CreateHandleToken();

    GVariant* activationReply = g_dbus_connection_call_sync(
        connection,
        DbusBusName,
        DbusObjectPath,
        DbusInterface,
        "StartServiceByName",
        g_variant_new("(su)", PortalBusName, static_cast<guint32>(0)),
        G_VARIANT_TYPE("(u)"),
        G_DBUS_CALL_FLAGS_NONE,
        PortalActivationTimeout,
        nullptr,
        &error
        );
    if (activationReply)
    {
        g_variant_unref(activationReply);
    }

    GVariant* ownerReply = error
        ? nullptr
        : g_dbus_connection_call_sync(
            connection,
            DbusBusName,
            DbusObjectPath,
            DbusInterface,
            "GetNameOwner",
            g_variant_new("(s)", PortalBusName),
            G_VARIANT_TYPE("(s)"),
            G_DBUS_CALL_FLAGS_NONE,
            PortalActivationTimeout,
            nullptr,
            &error
            );
    if (ownerReply)
    {
        const gchar* owner = nullptr;
        g_variant_get(ownerReply, "(&s)", &owner);
        if (owner)
        {
            response.portalOwner = owner;
        }
        g_variant_unref(ownerReply);
    }

    if (error || response.portalOwner.empty())
    {
        if (error)
        {
            g_error_free(error);
        }
        g_object_unref(connection);
        g_main_context_pop_thread_default(context);
        g_main_context_unref(context);
        g_variant_unref(options);
        return false;
    }

    guint ownerSubscription = g_dbus_connection_signal_subscribe(
        connection,
        DbusBusName,
        DbusInterface,
        "NameOwnerChanged",
        DbusObjectPath,
        PortalBusName,
        G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE,
        PortalOwnerChanged,
        &response,
        nullptr
        );
    guint responseSubscription = g_dbus_connection_signal_subscribe(
        connection,
        response.portalOwner.c_str(),
        PortalRequestInterface,
        "Response",
        nullptr,
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        PortalResponseReceived,
        &response,
        nullptr
        );

    GVariantBuilder requestOptions;
    g_variant_builder_init(&requestOptions, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&requestOptions, "{sv}", "handle_token", g_variant_new_string(token.c_str()));

    GVariantIter iterator;
    const gchar* key = nullptr;
    GVariant* value = nullptr;
    g_variant_iter_init(&iterator, options);
    while (g_variant_iter_next(&iterator, "{&sv}", &key, &value))
    {
        g_variant_builder_add(&requestOptions, "{sv}", key, value);
        g_variant_unref(value);
    }

    GCancellable* cancellable = g_cancellable_new();
    response.callStarted = true;
    g_dbus_connection_call(
        connection,
        response.portalOwner.c_str(),
        PortalObjectPath,
        PortalFileChooserInterface,
        method,
        g_variant_new("(ss@a{sv})", "", title.c_str(), g_variant_builder_end(&requestOptions)),
        G_VARIANT_TYPE("(o)"),
        G_DBUS_CALL_FLAGS_NONE,
        PortalMethodTimeout,
        cancellable,
        PortalMethodReturned,
        &response
        );
    g_main_context_pop_thread_default(context);

    bool localAbort = false;
    while (!response.received &&
        !response.failed &&
        !response.ownerLost &&
        !g_dbus_connection_is_closed(connection))
    {
        DispatchPortalContext(context);
        if (response.received || response.failed || response.ownerLost)
        {
            break;
        }
        if (!RunWgacModalCycle())
        {
            localAbort = true;
            break;
        }
    }

    if (response.callStarted && !response.callCompleted)
    {
        g_cancellable_cancel(cancellable);
        do
        {
            g_main_context_push_thread_default(context);
            g_main_context_iteration(context, TRUE);
            g_main_context_pop_thread_default(context);
        } while (!response.callCompleted);
    }

    auto connectionClosed = g_dbus_connection_is_closed(connection);
    if (!response.received &&
        response.requestCreated &&
        !response.ownerLost &&
        !connectionClosed)
    {
        GError* closeError = nullptr;
        GVariant* closeReply = g_dbus_connection_call_sync(
            connection,
            response.portalOwner.c_str(),
            response.requestPath.c_str(),
            PortalRequestInterface,
            "Close",
            g_variant_new("()"),
            nullptr,
            G_DBUS_CALL_FLAGS_NONE,
            PortalCloseTimeout,
            nullptr,
            &closeError
            );
        if (closeReply)
        {
            g_variant_unref(closeReply);
        }
        if (closeError)
        {
            g_error_free(closeError);
        }
    }

    g_main_context_push_thread_default(context);
    g_dbus_connection_signal_unsubscribe(connection, responseSubscription);
    g_dbus_connection_signal_unsubscribe(connection, ownerSubscription);
    while (g_main_context_iteration(context, FALSE))
    {
    }
    g_main_context_pop_thread_default(context);

    g_object_unref(cancellable);
    g_object_unref(connection);
    g_main_context_unref(context);
    g_variant_unref(options);

    if (localAbort || !response.received)
    {
        return false;
    }

    responseCode = response.response;
    responseResults = response.TakeResults();
    return true;
}

}

INativeDialogService::MessageBoxButtonsOutput WGacDialogService::ShowMessageBox(
    INativeWindow* window,
    const WString& text,
    const WString& title,
    MessageBoxButtonsInput buttons,
    MessageBoxDefaultButton defaultButton,
    MessageBoxIcons icon,
    MessageBoxModalOptions modal)
{
    // TODO: Implement native dialog
    return MessageBoxButtonsOutput::SelectOK;
}

bool WGacDialogService::ShowColorDialog(
    INativeWindow* window,
    Color& selection,
    bool selected,
    ColorDialogCustomColorOptions customColorOptions,
    Color* customColors)
{
    // TODO: Implement
    return false;
}

bool WGacDialogService::ShowFontDialog(
    INativeWindow* window,
    FontProperties& selectionFont,
    Color& selectionColor,
    bool selected,
    bool showEffect,
    bool forceFontExist)
{
    // TODO: Implement
    return false;
}

bool WGacDialogService::ShowFileDialog(
    INativeWindow* window,
    collections::List<WString>& selectionFileNames,
    vint& selectionFilterIndex,
    FileDialogTypes dialogType,
    const WString& title,
    const WString& initialFileName,
    const WString& initialDirectory,
    const WString& defaultExtension,
    const WString& filter,
    FileDialogOptions options)
{
    (void)window;

    ScopedFileDialog fileDialog;
    if (!fileDialog.IsAcquired())
    {
        return false;
    }

    bool save = dialogType == FileDialogSave || dialogType == FileDialogSavePreview;
    auto parsedFilters = ParseFilters(filter);

    GVariantBuilder portalOptions;
    g_variant_builder_init(&portalOptions, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&portalOptions, "{sv}", "modal", g_variant_new_boolean(TRUE));

    if (!save)
    {
        auto multiple = (options & FileDialogAllowMultipleSelection) != None;
        g_variant_builder_add(&portalOptions, "{sv}", "multiple", g_variant_new_boolean(multiple));
    }

    if (!parsedFilters.empty())
    {
        GVariantBuilder filters;
        g_variant_builder_init(&filters, G_VARIANT_TYPE("a(sa(us))"));
        for (const auto& parsedFilter : parsedFilters)
        {
            g_variant_builder_add_value(&filters, CreatePortalFilter(parsedFilter));
        }
        g_variant_builder_add(&portalOptions, "{sv}", "filters", g_variant_builder_end(&filters));

        if (selectionFilterIndex >= 0 && static_cast<size_t>(selectionFilterIndex) < parsedFilters.size())
        {
            g_variant_builder_add(
                &portalOptions,
                "{sv}",
                "current_filter",
                CreatePortalFilter(parsedFilters[static_cast<size_t>(selectionFilterIndex)])
                );
        }
    }

    auto effectiveDefaultExtension = GetEffectiveDefaultExtension(
        parsedFilters,
        selectionFilterIndex,
        defaultExtension
        );
    if (!AddInitialFileOptions(
        portalOptions,
        save,
        initialFileName,
        initialDirectory,
        effectiveDefaultExtension
        ))
    {
        g_variant_builder_clear(&portalOptions);
        return false;
    }

    auto effectiveTitle = ToUtf8(title);
    if (effectiveTitle.empty())
    {
        effectiveTitle = save ? "Save File" : "Open File";
    }

    guint responseCode = 2;
    GVariant* responseResults = nullptr;
    if (!RunPortalFileChooser(
        save ? "SaveFile" : "OpenFile",
        effectiveTitle,
        g_variant_builder_end(&portalOptions),
        responseCode,
        responseResults
        ))
    {
        return false;
    }

    bool accepted = false;
    if (responseCode == 0 && responseResults)
    {
        GVariant* uris = g_variant_lookup_value(responseResults, "uris", G_VARIANT_TYPE_STRING_ARRAY);
        if (uris)
        {
            gsize uriCount = 0;
            const gchar** uriValues = g_variant_get_strv(uris, &uriCount);
            collections::List<WString> selectedFiles;
            bool conversionFailed = false;
            for (gsize i = 0; i < uriCount; i++)
            {
                GError* error = nullptr;
                gchar* fileName = g_filename_from_uri(uriValues[i], nullptr, &error);
                if (!fileName)
                {
                    conversionFailed = true;
                }
                else
                {
                    GError* conversionError = nullptr;
                    gsize bytesWritten = 0;
                    gchar* fileNameUtf8 = g_filename_to_utf8(
                        fileName,
                        -1,
                        nullptr,
                        &bytesWritten,
                        &conversionError
                        );
                    if (fileNameUtf8)
                    {
                        selectedFiles.Add(FromUtf8(fileNameUtf8, bytesWritten));
                        g_free(fileNameUtf8);
                    }
                    else
                    {
                        conversionFailed = true;
                    }
                    if (conversionError)
                    {
                        g_error_free(conversionError);
                    }
                    g_free(fileName);
                }
                if (error)
                {
                    g_error_free(error);
                }
                if (conversionFailed)
                {
                    break;
                }
            }
            g_free(const_cast<gchar**>(uriValues));
            g_variant_unref(uris);

            if (!conversionFailed && selectedFiles.Count() > 0)
            {
                selectionFileNames.Clear();
                for (vint i = 0; i < selectedFiles.Count(); i++)
                {
                    selectionFileNames.Add(selectedFiles[i]);
                }
                accepted = true;
            }
        }

        GVariant* currentFilter = g_variant_lookup_value(
            responseResults,
            "current_filter",
            G_VARIANT_TYPE("(sa(us))")
            );
        if (accepted && currentFilter)
        {
            for (size_t i = 0; i < parsedFilters.size(); i++)
            {
                GVariant* candidate = g_variant_ref_sink(CreatePortalFilter(parsedFilters[i]));
                bool matched = g_variant_equal(currentFilter, candidate);
                g_variant_unref(candidate);
                if (matched)
                {
                    selectionFilterIndex = static_cast<vint>(i);
                    break;
                }
            }
            g_variant_unref(currentFilter);
        }
        else if (currentFilter)
        {
            g_variant_unref(currentFilter);
        }
    }

    if (responseResults)
    {
        g_variant_unref(responseResults);
    }
    return accepted;
}

}
}
}
