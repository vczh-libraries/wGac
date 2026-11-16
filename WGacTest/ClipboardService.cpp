#include "Services/WGacClipboardService.h"
#include "Services/WGacImageService.h"

using namespace vl;
using namespace vl::presentation;
using namespace vl::presentation::wayland;

TEST_FILE
{
    TEST_CASE(L"Local copy is readable without a compositor input serial")
    {
        WGacClipboardService service;
        auto writer = service.WriteClipboard();
        writer->SetText(L"Clipboard 中文 🙂");
        TEST_ASSERT(writer->Submit());
        TEST_ASSERT(!writer->Submit());
        auto reader = service.ReadClipboard();
        TEST_ASSERT(reader->ContainsText());
        TEST_ASSERT(reader->GetText() == L"Clipboard 中文 🙂");

        auto empty = service.WriteClipboard();
        empty->SetText(L"");
        TEST_ASSERT(empty->Submit());
        TEST_ASSERT(service.ReadClipboard()->ContainsText());
        TEST_ASSERT(service.ReadClipboard()->GetText() == L"");
        TEST_ASSERT(reader->GetText() == L"Clipboard 中文 🙂");
        service.Cleanup();
    });

    TEST_CASE(L"Rich documents and images survive local clipboard roundtrips")
    {
        WGacClipboardService service;
        auto document = Ptr(new DocumentModel);
        auto paragraph = Ptr(new DocumentParagraphRun);
        paragraph->alignment = Alignment::Right;
        auto text = Ptr(new DocumentTextRun);
        text->text = L"Rich 中文 🙂";
        paragraph->runs.Add(text);
        document->paragraphs.Add(paragraph);
        auto writer = service.WriteClipboard();
        writer->SetDocument(document);
        TEST_ASSERT(writer->Submit());
        auto reader = service.ReadClipboard();
        TEST_ASSERT(reader->ContainsDocument());
        TEST_ASSERT(reader->GetDocument()->paragraphs[0]->alignment.Value() == Alignment::Right);
        TEST_ASSERT(reader->GetText() == L"Rich 中文 🙂");

        auto image = Ptr(new WGacImage(nullptr, cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 3, 2)));
        auto imageWriter = service.WriteClipboard();
        imageWriter->SetImage(image);
        TEST_ASSERT(imageWriter->Submit());
        reader = service.ReadClipboard();
        TEST_ASSERT(reader->ContainsImage());
        TEST_ASSERT(reader->GetImage()->GetFrame(0)->GetSize() == Size(3, 2));
        TEST_ASSERT(!reader->ContainsText());
        TEST_ASSERT(!reader->ContainsDocument());
        service.Cleanup();
    });

    TEST_CASE(L"A compositor selection clear retires an unpublished local copy")
    {
        WGacClipboardService service;
        auto writer = service.WriteClipboard();
        writer->SetText(L"old selection");
        TEST_ASSERT(writer->Submit());
        WGacClipboardService::data_device_selection(&service, nullptr, nullptr);
        auto reader = service.ReadClipboard();
        TEST_ASSERT(!reader->ContainsText());
        TEST_ASSERT(!reader->ContainsDocument());
        TEST_ASSERT(!reader->ContainsImage());
        service.Cleanup();
    });
}

void GuiMain()
{
}

int main(int argc, char* argv[])
{
    auto result = unittest::UnitTest::RunAndDisposeTests(argc, argv);
    FinalizeGlobalStorage();
    return result;
}
