#include "TuiWGacController.h"
#include "TuiWGacClipboardService.h"
#include "../Services/WGacResourceService.h"
#include "../Services/WGacInputService.h"
#include "../Services/WGacImageService.h"
#include <unistd.h>
#include <limits.h>
#include <cerrno>
#include <clocale>

namespace vl::presentation::wayland
{
	class TuiWGacResourceService : public WGacResourceService
	{
	public:
		FontProperties GetDefaultFont() override
		{
			auto font = defaultFont;
			font.fontFamily = L"TuiFont";
			font.size = 1;
			return font;
		}

		void SetDefaultFont(const FontProperties& value) override
		{
			defaultFont = value;
			defaultFont.fontFamily = L"TuiFont";
			defaultFont.size = 1;
		}

		void EnumerateFonts(collections::List<WString>& fonts) override
		{
			fonts.Add(L"TuiFont");
		}
	};

	class TuiWGacInputService : public WGacInputService
	{
	public:
		TuiWGacInputService() : WGacInputService(nullptr) {}

		void StartTimer() override
		{
			isTimerEnabled = true;
			console::TUI::StartTimer(16);
		}

		void StopTimer() override
		{
			isTimerEnabled = false;
			if (console::TUI::IsInUse()) console::TUI::StopTimer();
		}
	};

	class TuiWGacController : public TuiControllerBase
	{
	protected:
		TuiWGacResourceService	resourceService;
		TuiWGacInputService		inputService;
		WGacImageService			imageService;
		TuiWGacClipboardService	clipboardService;

		void PumpPlatformEvents() override
		{
			if (clipboardService.PumpEvents()) ClipboardUpdated();
		}

	public:
		TuiWGacController(const TuiConfiguration& configuration)
			: TuiControllerBase(configuration)
		{
		}

		~TuiWGacController()
		{
			inputService.StopTimer();
		}

		INativeResourceService* ResourceService() override { return &resourceService; }
		INativeInputService* InputService() override { return &inputService; }
		INativeClipboardService* ClipboardService() override { return &clipboardService; }
		INativeImageService* ImageService() override { return &imageService; }

		WString GetExecutablePath() override
		{
			char path[PATH_MAX];
			auto length = readlink("/proc/self/exe", path, sizeof(path));
			CHECK_ERROR(length > 0 && length < sizeof(path), L"TuiWGacController#Failed to read executable path.");
			return atow(AString::CopyFrom(path, length));
		}

		void ApplyTitle(const WString& title) override
		{
			// OSC titles cannot contain terminal control characters.
			WString sanitized;
			for (vint i = 0; i < title.Length(); i++)
			{
				auto c = title[i];
				if (c >= 0x20 && !(c >= 0x7F && c < 0xA0)) sanitized += WString::FromChar(c);
			}
			auto output = wtou8(L"\x1B]2;" + sanitized + L"\x07");
			vint offset = 0;
			while (offset < output.Length())
			{
				auto written = write(STDOUT_FILENO, output.Buffer() + offset, output.Length() - offset);
				if (written == -1 && errno == EINTR) continue;
				CHECK_ERROR(written > 0, L"TuiWGacController#Failed to set terminal title.");
				offset += written;
			}
		}
	};

	int SetupTuiWaylandRenderer(const TuiConfiguration& configuration)
	{
		// Native file/image services use the locale's multibyte conversion.
		// Initialize it before creating services or starting worker threads.
		CHECK_ERROR(std::setlocale(LC_CTYPE, "") != nullptr, L"SetupTuiWaylandRenderer#Failed to initialize the character locale.");
		TuiWGacController controller(configuration);
		console::TUI::InstallListener(&controller);
		try
		{
			console::TUI::Start({});
		}
		catch (...)
		{
			console::TUI::UninstallListener(&controller);
			throw;
		}
		console::TUI::UninstallListener(&controller);
		return 0;
	}
}
