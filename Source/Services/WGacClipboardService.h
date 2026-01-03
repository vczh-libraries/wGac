#ifndef WGAC_CLIPBOARDSERVICE_H
#define WGAC_CLIPBOARDSERVICE_H

#include "GacUI.h"
#include <wayland-client.h>
#include <string>
#include <mutex>

namespace vl {
namespace presentation {
namespace wayland {

class WGacClipboardService : public Object, public INativeClipboardService
{
    friend class WGacClipboardWriter;
    friend class WGacClipboardReader;

private:
    // Current clipboard offer from compositor (for reading)
    wl_data_offer* current_offer = nullptr;
    bool offer_has_text = false;
    bool offer_has_image = false;
    std::string offer_text_mime;
    std::string offer_image_mime;

    // Current data source (for writing - we own this)
    wl_data_source* current_source = nullptr;
    std::string pending_text;
    Ptr<INativeImage> pending_image;

    std::mutex mutex;

public:
    // Static callbacks (must be public for C linkage in listener structs)
    // Data device listener
    static void data_device_data_offer(void* data, wl_data_device* device, wl_data_offer* offer);
    static void data_device_enter(void* data, wl_data_device* device, uint32_t serial,
                                  wl_surface* surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer* offer);
    static void data_device_leave(void* data, wl_data_device* device);
    static void data_device_motion(void* data, wl_data_device* device, uint32_t time,
                                   wl_fixed_t x, wl_fixed_t y);
    static void data_device_drop(void* data, wl_data_device* device);
    static void data_device_selection(void* data, wl_data_device* device, wl_data_offer* offer);

    // Data offer listener
    static void data_offer_offer(void* data, wl_data_offer* offer, const char* mime_type);
    static void data_offer_source_actions(void* data, wl_data_offer* offer, uint32_t actions);
    static void data_offer_action(void* data, wl_data_offer* offer, uint32_t action);

    // Data source listener
    static void data_source_target(void* data, wl_data_source* source, const char* mime_type);
    static void data_source_send(void* data, wl_data_source* source, const char* mime_type, int32_t fd);
    static void data_source_cancelled(void* data, wl_data_source* source);
    static void data_source_dnd_drop_performed(void* data, wl_data_source* source);
    static void data_source_dnd_finished(void* data, wl_data_source* source);
    static void data_source_action(void* data, wl_data_source* source, uint32_t action);
    WGacClipboardService();
    virtual ~WGacClipboardService();

    void Initialize();
    void Cleanup();  // Call before display disconnect

    Ptr<INativeClipboardReader> ReadClipboard() override;
    Ptr<INativeClipboardWriter> WriteClipboard() override;
};

}
}
}

#endif // WGAC_CLIPBOARDSERVICE_H
