#include "WGacClipboardService.h"

namespace vl {
namespace presentation {
namespace wayland {

class WGacClipboardWriter : public Object, public INativeClipboardWriter
{
public:
    WGacClipboardWriter(WGacClipboardService* service) {}

    void SetText(const WString& value) override
    {
        // TODO: Implement Wayland clipboard
    }

    void SetDocument(Ptr<DocumentModel> value) override
    {
        // TODO: Implement
    }

    void SetImage(Ptr<INativeImage> value) override
    {
        // TODO: Implement
    }

    bool Submit() override
    {
        return true;
    }
};

class WGacClipboardReader : public Object, public INativeClipboardReader
{
public:
    WGacClipboardReader(WGacClipboardService* service) {}

    bool ContainsText() override
    {
        return false;
    }

    WString GetText() override
    {
        return L"";
    }

    bool ContainsDocument() override
    {
        return false;
    }

    Ptr<DocumentModel> GetDocument() override
    {
        return Ptr<DocumentModel>();
    }

    bool ContainsImage() override
    {
        return false;
    }

    Ptr<INativeImage> GetImage() override
    {
        return Ptr<INativeImage>();
    }
};

WGacClipboardService::WGacClipboardService()
{
}

WGacClipboardService::~WGacClipboardService()
{
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
