#include "WGacClipboardService.h"
#include "../Wayland/WaylandDisplay.h"
#include "../Wayland/WaylandSeat.h"
#include "../WGacController.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <vector>

namespace vl {
namespace presentation {
namespace wayland {

namespace {
    // MIME types we support
    const char* TEXT_MIME_PLAIN = "text/plain";
    const char* TEXT_MIME_UTF8 = "text/plain;charset=utf-8";
    const char* TEXT_MIME_UTF8_ALT = "UTF8_STRING";
    const char* TEXT_MIME_STRING = "STRING";
    const char* DOCUMENT_MIME = "application/x-gacui-document";
    const char* IMAGE_MIME_PNG = "image/png";

    // Data offer listener
    const wl_data_offer_listener data_offer_listener = {
        .offer = WGacClipboardService::data_offer_offer,
        .source_actions = WGacClipboardService::data_offer_source_actions,
        .action = WGacClipboardService::data_offer_action,
    };

    // Data device listener
    const wl_data_device_listener data_device_listener = {
        .data_offer = WGacClipboardService::data_device_data_offer,
        .enter = WGacClipboardService::data_device_enter,
        .leave = WGacClipboardService::data_device_leave,
        .motion = WGacClipboardService::data_device_motion,
        .drop = WGacClipboardService::data_device_drop,
        .selection = WGacClipboardService::data_device_selection,
    };

    // Data source listener
    const wl_data_source_listener data_source_listener = {
        .target = WGacClipboardService::data_source_target,
        .send = WGacClipboardService::data_source_send,
        .cancelled = WGacClipboardService::data_source_cancelled,
        .dnd_drop_performed = WGacClipboardService::data_source_dnd_drop_performed,
        .dnd_finished = WGacClipboardService::data_source_dnd_finished,
        .action = WGacClipboardService::data_source_action,
    };

    bool IsSupportedTextMime(const char* mime) {
        return strcmp(mime, TEXT_MIME_PLAIN) == 0 ||
               strcmp(mime, TEXT_MIME_UTF8) == 0 ||
               strcmp(mime, TEXT_MIME_UTF8_ALT) == 0 ||
               strcmp(mime, TEXT_MIME_STRING) == 0;
    }

    bool IsSupportedImageMime(const char* mime) {
        return strcmp(mime, IMAGE_MIME_PNG) == 0;
    }

    // Helper to read all data from fd
    std::vector<char> ReadFromFd(int fd) {
        std::vector<char> data;
        char buffer[4096];
        ssize_t n;
        while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
            data.insert(data.end(), buffer, buffer + n);
        }
        close(fd);
        return data;
    }

    // Helper to write data to fd
    void WriteToFd(int fd, const void* data, size_t size) {
        const char* ptr = static_cast<const char*>(data);
        size_t remaining = size;
        while (remaining > 0) {
            ssize_t written = write(fd, ptr, remaining);
            if (written <= 0) break;
            ptr += written;
            remaining -= written;
        }
        close(fd);
    }

    // Convert WString to UTF-8
    std::string WStringToUtf8(const WString& wstr) {
        if (wstr.Length() == 0) return "";

        std::string result;
        const wchar_t* ptr = wstr.Buffer();
        size_t len = wstr.Length();

        for (size_t i = 0; i < len; i++) {
            wchar_t ch = ptr[i];
            if (ch < 0x80) {
                result += static_cast<char>(ch);
            } else if (ch < 0x800) {
                result += static_cast<char>(0xC0 | (ch >> 6));
                result += static_cast<char>(0x80 | (ch & 0x3F));
            } else if (ch < 0x10000) {
                result += static_cast<char>(0xE0 | (ch >> 12));
                result += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (ch & 0x3F));
            } else {
                result += static_cast<char>(0xF0 | (ch >> 18));
                result += static_cast<char>(0x80 | ((ch >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (ch & 0x3F));
            }
        }
        return result;
    }

    // Convert UTF-8 to WString
    WString Utf8ToWString(const std::vector<char>& utf8) {
        if (utf8.empty()) return L"";

        std::wstring result;
        const unsigned char* ptr = reinterpret_cast<const unsigned char*>(utf8.data());
        size_t len = utf8.size();

        for (size_t i = 0; i < len; ) {
            unsigned char ch = ptr[i];
            wchar_t wch;

            if ((ch & 0x80) == 0) {
                wch = ch;
                i += 1;
            } else if ((ch & 0xE0) == 0xC0 && i + 1 < len) {
                wch = ((ch & 0x1F) << 6) | (ptr[i+1] & 0x3F);
                i += 2;
            } else if ((ch & 0xF0) == 0xE0 && i + 2 < len) {
                wch = ((ch & 0x0F) << 12) | ((ptr[i+1] & 0x3F) << 6) | (ptr[i+2] & 0x3F);
                i += 3;
            } else if ((ch & 0xF8) == 0xF0 && i + 3 < len) {
                uint32_t cp = ((ch & 0x07) << 18) | ((ptr[i+1] & 0x3F) << 12) |
                              ((ptr[i+2] & 0x3F) << 6) | (ptr[i+3] & 0x3F);
                // For wchar_t on Linux (32-bit), we can store full codepoint
                wch = static_cast<wchar_t>(cp);
                i += 4;
            } else {
                wch = L'?';
                i += 1;
            }
            result += wch;
        }
        return WString(result.c_str());
    }
}

// Retain private document/image objects while this process owns the selection.
// Reading our own copy must not depend on a compositor roundtrip or input serial.
class WGacClipboardSnapshot : public Object, public INativeClipboardReader
{
public:
    Nullable<WString> text;
    Ptr<DocumentModel> document;
    Ptr<INativeImage> image;

    bool ContainsText() override { return text; }
    WString GetText() override { return text ? text.Value() : WString(); }
    bool ContainsDocument() override { return document; }
    Ptr<DocumentModel> GetDocument() override { return document; }
    bool ContainsImage() override { return image; }
    Ptr<INativeImage> GetImage() override { return image; }
};

// WGacClipboardWriter implementation
class WGacClipboardWriter : public Object, public INativeClipboardWriter
{
private:
    WGacClipboardService* service;
    WString text;
    Ptr<DocumentModel> document;
    Ptr<INativeImage> image;
    bool hasText = false;
    bool hasDocument = false;
    bool hasImage = false;
    bool submitted = false;

public:
    WGacClipboardWriter(WGacClipboardService* _service) : service(_service) {}

    void SetText(const WString& value) override
    {
        text = value;
        hasText = true;
    }

    void SetDocument(Ptr<DocumentModel> value) override
    {
        document = value;
        hasDocument = true;
    }

    void SetImage(Ptr<INativeImage> value) override
    {
        image = value;
        hasImage = true;
    }

    bool Submit() override
    {
        if (submitted) return false;
        submitted = true;
        auto snapshot = Ptr(new WGacClipboardSnapshot);
        if (hasText) snapshot->text = text;
        if (hasDocument) {
            snapshot->document = document;
            ModifyDocumentForClipboard(document);
            if (!hasText) snapshot->text = document->GetTextForReading(L"\n");
            if (!hasImage) snapshot->image = GetImageFromSingleImageDocument(document);
        }
        if (hasImage) snapshot->image = image;
        auto documentBytes = snapshot->document ? Ptr(new stream::MemoryStream) : nullptr;
        if (documentBytes) SaveDocumentToClipboardStream(snapshot->document, *documentBytes.Obj());
        auto* display = GetWaylandDisplay();
        auto serial = display && display->GetWaylandSeat() ? display->GetWaylandSeat()->GetLastInputSerial() : 0;
        bool canPublish = serial && display->GetDataDeviceManager() && display->GetDataDevice();
        if (service->require_system_write && !canPublish) return false;
        std::unique_lock<std::mutex> lock(service->mutex);

        // Destroy previous source
        if (service->current_source) {
            wl_data_source_destroy(service->current_source);
            service->current_source = nullptr;
        }

        service->local_clipboard = snapshot;
        service->pending_text = WStringToUtf8(snapshot->GetText());
        service->pending_image = snapshot->image;
        service->pending_document = documentBytes;
        service->local_mime = "application/x-gacui-selection-" + std::to_string(getpid())
            + "-" + std::to_string(++service->selection_version);

        // Automation has no compositor input serial. Keep its copy process-local;
        // ordinary input additionally publishes the standard external formats.
        if (canPublish) {
            service->current_source = wl_data_device_manager_create_data_source(display->GetDataDeviceManager());
            CHECK_ERROR(service->current_source, L"Failed to create the Wayland clipboard source.");
            wl_data_source_add_listener(service->current_source, &data_source_listener, service);
            wl_data_source_offer(service->current_source, service->local_mime.c_str());
            if (snapshot->ContainsText()) {
                wl_data_source_offer(service->current_source, TEXT_MIME_UTF8);
                wl_data_source_offer(service->current_source, TEXT_MIME_PLAIN);
                wl_data_source_offer(service->current_source, TEXT_MIME_UTF8_ALT);
                wl_data_source_offer(service->current_source, TEXT_MIME_STRING);
            }
            if (documentBytes) wl_data_source_offer(service->current_source, DOCUMENT_MIME);
            if (snapshot->ContainsImage()) wl_data_source_offer(service->current_source, IMAGE_MIME_PNG);
            wl_data_device_set_selection(display->GetDataDevice(), service->current_source, serial);
            display->Flush();
        }
        lock.unlock();
        if (auto* controller = GetWGacController()) {
            controller->CallbackService()->Invoker()->InvokeClipboardUpdated();
        }
        return true;
    }
};

// WGacClipboardReader implementation
class WGacClipboardReader : public Object, public INativeClipboardReader
{
    WGacClipboardService* service;
    wl_data_offer* offer;
    WGacClipboardService::Formats formats;

    std::vector<char> Read(const std::string& mime)
    {
        if (mime.empty()) return {};
        int descriptors[2];
        {
            std::lock_guard<std::mutex> lock(service->mutex);
            if (!offer || offer != service->current_offer || pipe(descriptors) < 0) return {};
            wl_data_offer_receive(offer, mime.c_str(), descriptors[1]);
            close(descriptors[1]);
        }
        GetWaylandDisplay()->Flush();
        return ReadFromFd(descriptors[0]);
    }
public:
    WGacClipboardReader(WGacClipboardService* owner)
        : service(owner), offer(owner->current_offer), formats(owner->current_formats) {}

    bool ContainsText() override { return !formats.text.empty(); }
    WString GetText() override { return Utf8ToWString(Read(formats.text)); }
    bool ContainsDocument() override { return formats.document; }
    Ptr<DocumentModel> GetDocument() override
    {
        auto bytes = Read(formats.document ? DOCUMENT_MIME : "");
        if (bytes.empty()) return nullptr;
        stream::MemoryWrapperStream input(bytes.data(), static_cast<vint>(bytes.size()));
        return LoadDocumentFromClipboardStream(input);
    }
    bool ContainsImage() override { return formats.image; }
    Ptr<INativeImage> GetImage() override
    {
        auto bytes = Read(formats.image ? IMAGE_MIME_PNG : "");
        if (bytes.empty()) return nullptr;
        return GetCurrentController()->ImageService()->CreateImageFromMemory(bytes.data(), static_cast<vint>(bytes.size()));
    }
};

// Data device listener callbacks
void WGacClipboardService::data_device_data_offer(void* data, wl_data_device* /*device*/, wl_data_offer* offer)
{
    // New offer received - add listener to track mime types
    wl_data_offer_add_listener(offer, &data_offer_listener, data);
}

void WGacClipboardService::data_device_enter(void* /*data*/, wl_data_device* /*device*/, uint32_t /*serial*/,
                                              wl_surface* /*surface*/, wl_fixed_t /*x*/, wl_fixed_t /*y*/,
                                              wl_data_offer* /*offer*/)
{
    // DnD enter - not used for clipboard
}

void WGacClipboardService::data_device_leave(void* /*data*/, wl_data_device* /*device*/)
{
    // DnD leave - not used for clipboard
}

void WGacClipboardService::data_device_motion(void* /*data*/, wl_data_device* /*device*/, uint32_t /*time*/,
                                               wl_fixed_t /*x*/, wl_fixed_t /*y*/)
{
    // DnD motion - not used for clipboard
}

void WGacClipboardService::data_device_drop(void* /*data*/, wl_data_device* /*device*/)
{
    // DnD drop - not used for clipboard
}

void WGacClipboardService::data_device_selection(void* data, wl_data_device* /*device*/, wl_data_offer* offer)
{
    auto* self = static_cast<WGacClipboardService*>(data);
    std::unique_lock<std::mutex> lock(self->mutex);

    // Destroy old offer
    if (self->current_offer && self->current_offer != offer) {
        self->offers.erase(self->current_offer);
        wl_data_offer_destroy(self->current_offer);
    }

    self->current_offer = offer;
    self->current_formats = offer ? self->offers[offer] : Formats{};
    if ((offer && !self->current_formats.local) || (!offer && !self->current_source)) self->local_clipboard = nullptr;
    lock.unlock();
    if (auto* controller = GetWGacController()) {
        controller->CallbackService()->Invoker()->InvokeClipboardUpdated();
    }
}

// Data offer listener callbacks
void WGacClipboardService::data_offer_offer(void* data, wl_data_offer* offer, const char* mime_type)
{
    auto* self = static_cast<WGacClipboardService*>(data);
    std::lock_guard<std::mutex> lock(self->mutex);

    auto& formats = self->offers[offer];
    if (mime_type == self->local_mime) formats.local = true;
    if (IsSupportedTextMime(mime_type) && (formats.text.empty() || strcmp(mime_type, TEXT_MIME_UTF8) == 0)) formats.text = mime_type;
    if (IsSupportedImageMime(mime_type)) formats.image = true;
    if (strcmp(mime_type, DOCUMENT_MIME) == 0) formats.document = true;
}

void WGacClipboardService::data_offer_source_actions(void* /*data*/, wl_data_offer* /*offer*/, uint32_t /*actions*/)
{
    // DnD actions - not used for clipboard
}

void WGacClipboardService::data_offer_action(void* /*data*/, wl_data_offer* /*offer*/, uint32_t /*action*/)
{
    // DnD action - not used for clipboard
}

// Data source listener callbacks
void WGacClipboardService::data_source_target(void* /*data*/, wl_data_source* /*source*/, const char* /*mime_type*/)
{
    // Target accepted/rejected mime type - informational only
}

void WGacClipboardService::data_source_send(void* data, wl_data_source* source, const char* mime_type, int32_t fd)
{
    auto* self = static_cast<WGacClipboardService*>(data);
    std::lock_guard<std::mutex> lock(self->mutex);

    if (source != self->current_source) { close(fd); return; }
    if (strcmp(mime_type, DOCUMENT_MIME) == 0 && self->pending_document) {
        WriteToFd(fd, static_cast<const char*>(self->pending_document->GetInternalBuffer()), self->pending_document->Size());
    } else if (IsSupportedTextMime(mime_type)) {
        WriteToFd(fd, self->pending_text.c_str(), self->pending_text.size());
    } else if (IsSupportedImageMime(mime_type) && self->pending_image) {
        stream::MemoryStream memStream;
        self->pending_image->SaveToStream(memStream, INativeImage::Png);
        memStream.SeekFromBegin(0);
        auto size = memStream.Size();
        if (size > 0) {
            std::vector<char> buf(size);
            memStream.Read(buf.data(), size);
            WriteToFd(fd, buf.data(), size);
        } else {
            close(fd);
        }
    } else {
        close(fd);
    }
}

void WGacClipboardService::data_source_cancelled(void* data, wl_data_source* source)
{
    auto* self = static_cast<WGacClipboardService*>(data);
    std::lock_guard<std::mutex> lock(self->mutex);

    if (source && self->current_source == source) {
        wl_data_source_destroy(source);
        self->current_source = nullptr;
        self->pending_text.clear();
        self->pending_image = nullptr;
        self->pending_document = nullptr;
        self->local_clipboard = nullptr;
    }
}

void WGacClipboardService::data_source_dnd_drop_performed(void* /*data*/, wl_data_source* /*source*/)
{
    // DnD - not used for clipboard
}

void WGacClipboardService::data_source_dnd_finished(void* /*data*/, wl_data_source* /*source*/)
{
    // DnD - not used for clipboard
}

void WGacClipboardService::data_source_action(void* /*data*/, wl_data_source* /*source*/, uint32_t /*action*/)
{
    // DnD - not used for clipboard
}

// WGacClipboardService implementation
WGacClipboardService::WGacClipboardService()
{
}

WGacClipboardService::~WGacClipboardService()
{
    // Note: Cleanup() should be called before display disconnect
    // Don't destroy wayland objects here as display may already be disconnected
}

void WGacClipboardService::Cleanup()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (current_source) {
        wl_data_source_destroy(current_source);
        current_source = nullptr;
    }
    if (current_offer) {
        wl_data_offer_destroy(current_offer);
        current_offer = nullptr;
    }
    pending_text.clear();
    pending_image = nullptr;
    local_clipboard = nullptr;
    pending_document = nullptr;
    offers.clear();
    current_formats = {};
}

void WGacClipboardService::Initialize()
{
    auto* display = GetWaylandDisplay();
    if (display && display->GetDataDevice()) {
        wl_data_device_add_listener(display->GetDataDevice(), &data_device_listener, this);
    }
}

Ptr<INativeClipboardReader> WGacClipboardService::ReadClipboard()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (local_clipboard) return local_clipboard;
    return Ptr(new WGacClipboardReader(this));
}

Ptr<INativeClipboardWriter> WGacClipboardService::WriteClipboard()
{
    return Ptr(new WGacClipboardWriter(this));
}

}
}
}
