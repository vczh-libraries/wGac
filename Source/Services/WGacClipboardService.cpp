#include "WGacClipboardService.h"
#include "../Wayland/WaylandDisplay.h"
#include "../Wayland/WaylandSeat.h"
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
        auto* display = GetWaylandDisplay();
        if (!display || !display->GetDataDeviceManager()) {
            return false;
        }

        std::lock_guard<std::mutex> lock(service->mutex);

        // Destroy previous source
        if (service->current_source) {
            wl_data_source_destroy(service->current_source);
            service->current_source = nullptr;
        }

        // Create new data source
        service->current_source = wl_data_device_manager_create_data_source(
            display->GetDataDeviceManager());
        if (!service->current_source) {
            return false;
        }

        wl_data_source_add_listener(service->current_source, &data_source_listener, service);

        // Determine what to offer
        if (hasText) {
            service->pending_text = WStringToUtf8(text);
            wl_data_source_offer(service->current_source, TEXT_MIME_UTF8);
            wl_data_source_offer(service->current_source, TEXT_MIME_PLAIN);
            wl_data_source_offer(service->current_source, TEXT_MIME_UTF8_ALT);
            wl_data_source_offer(service->current_source, TEXT_MIME_STRING);
        } else if (hasDocument) {
            service->pending_text = WStringToUtf8(document->GetTextForReading(L"\n"));
            wl_data_source_offer(service->current_source, TEXT_MIME_UTF8);
            wl_data_source_offer(service->current_source, TEXT_MIME_PLAIN);
        }

        if (hasImage) {
            service->pending_image = image;
            wl_data_source_offer(service->current_source, IMAGE_MIME_PNG);
        }

        // Set selection - use most recent input serial from the seat
        auto* data_device = display->GetDataDevice();
        if (data_device) {
            uint32_t serial = 0;
            if (display->GetWaylandSeat()) {
                serial = display->GetWaylandSeat()->GetLastInputSerial();
            }
            wl_data_device_set_selection(data_device, service->current_source, serial);
            display->Flush();
            return true;
        }

        return false;
    }
};

// WGacClipboardReader implementation
class WGacClipboardReader : public Object, public INativeClipboardReader
{
private:
    WGacClipboardService* service;

public:
    WGacClipboardReader(WGacClipboardService* _service) : service(_service) {}

    bool ContainsText() override
    {
        std::lock_guard<std::mutex> lock(service->mutex);
        return service->offer_has_text;
    }

    WString GetText() override
    {
        wl_data_offer* offer = nullptr;
        std::string mime;

        // Copy what we need under the lock, then release before Wayland calls
        // to avoid deadlock when data_source_send callback tries to acquire mutex
        {
            std::lock_guard<std::mutex> lock(service->mutex);
            if (!service->current_offer || service->offer_text_mime.empty()) {
                return L"";
            }
            offer = service->current_offer;
            mime = service->offer_text_mime;
        }

        // Create pipe to receive data
        int pipefd[2];
        if (pipe(pipefd) < 0) {
            return L"";
        }

        // Request data (no lock held - avoids deadlock with data_source_send)
        wl_data_offer_receive(offer, mime.c_str(), pipefd[1]);
        close(pipefd[1]); // Close write end

        // Need to flush and dispatch to actually receive data
        auto* display = GetWaylandDisplay();
        if (display) {
            display->Flush();
            display->Roundtrip();
        }

        // Read data
        auto data = ReadFromFd(pipefd[0]);
        return Utf8ToWString(data);
    }

    bool ContainsDocument() override
    {
        // We could support RTF/HTML here in the future
        return false;
    }

    Ptr<DocumentModel> GetDocument() override
    {
        return Ptr<DocumentModel>();
    }

    bool ContainsImage() override
    {
        std::lock_guard<std::mutex> lock(service->mutex);
        return service->offer_has_image;
    }

    Ptr<INativeImage> GetImage() override
    {
        wl_data_offer* offer = nullptr;
        std::string mime;

        // Copy what we need under the lock, then release before Wayland calls
        // to avoid deadlock when data_source_send callback tries to acquire mutex
        {
            std::lock_guard<std::mutex> lock(service->mutex);
            if (!service->current_offer || service->offer_image_mime.empty()) {
                return Ptr<INativeImage>();
            }
            offer = service->current_offer;
            mime = service->offer_image_mime;
        }

        // Create pipe to receive data
        int pipefd[2];
        if (pipe(pipefd) < 0) {
            return Ptr<INativeImage>();
        }

        // Request data (no lock held - avoids deadlock with data_source_send)
        wl_data_offer_receive(offer, mime.c_str(), pipefd[1]);
        close(pipefd[1]);

        auto* display = GetWaylandDisplay();
        if (display) {
            display->Flush();
            display->Roundtrip();
        }

        // Read PNG data
        auto data = ReadFromFd(pipefd[0]);
        if (data.empty()) {
            return Ptr<INativeImage>();
        }

        // Load image from PNG data using image service
        auto* controller = GetCurrentController();
        if (controller) {
            auto* imageService = controller->ImageService();
            if (imageService) {
                // Create a memory stream from the PNG data
                stream::MemoryStream memStream;
                memStream.Write(data.data(), data.size());
                memStream.SeekFromBegin(0);
                return imageService->CreateImageFromStream(memStream);
            }
        }

        return Ptr<INativeImage>();
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
    std::lock_guard<std::mutex> lock(self->mutex);

    // Destroy old offer
    if (self->current_offer && self->current_offer != offer) {
        wl_data_offer_destroy(self->current_offer);
    }

    self->current_offer = offer;
    // Note: offer_has_text/offer_has_image were already set in data_offer_offer
    // but we need to reset them if offer is null
    if (!offer) {
        self->offer_has_text = false;
        self->offer_has_image = false;
        self->offer_text_mime.clear();
        self->offer_image_mime.clear();
    }
}

// Data offer listener callbacks
void WGacClipboardService::data_offer_offer(void* data, wl_data_offer* /*offer*/, const char* mime_type)
{
    auto* self = static_cast<WGacClipboardService*>(data);
    std::lock_guard<std::mutex> lock(self->mutex);

    if (IsSupportedTextMime(mime_type)) {
        self->offer_has_text = true;
        // Prefer UTF-8 mime type
        if (self->offer_text_mime.empty() ||
            strcmp(mime_type, TEXT_MIME_UTF8) == 0) {
            self->offer_text_mime = mime_type;
        }
    }
    if (IsSupportedImageMime(mime_type)) {
        self->offer_has_image = true;
        self->offer_image_mime = mime_type;
    }
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

void WGacClipboardService::data_source_send(void* data, wl_data_source* /*source*/, const char* mime_type, int32_t fd)
{
    auto* self = static_cast<WGacClipboardService*>(data);
    std::lock_guard<std::mutex> lock(self->mutex);

    if (IsSupportedTextMime(mime_type) && !self->pending_text.empty()) {
        WriteToFd(fd, self->pending_text.c_str(), self->pending_text.size());
    } else if (IsSupportedImageMime(mime_type) && self->pending_image) {
        // TODO: Encode image to PNG and write to fd
        // For now just close
        close(fd);
    } else {
        close(fd);
    }
}

void WGacClipboardService::data_source_cancelled(void* data, wl_data_source* source)
{
    auto* self = static_cast<WGacClipboardService*>(data);
    std::lock_guard<std::mutex> lock(self->mutex);

    if (self->current_source == source) {
        wl_data_source_destroy(source);
        self->current_source = nullptr;
        self->pending_text.clear();
        self->pending_image = nullptr;
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
    return Ptr(new WGacClipboardReader(this));
}

Ptr<INativeClipboardWriter> WGacClipboardService::WriteClipboard()
{
    return Ptr(new WGacClipboardWriter(this));
}

}
}
}
