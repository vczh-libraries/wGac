#include "WGacController.h"
#include "WGacNativeWindow.h"
#include "Services/WGacAsyncService.h"
#include "Services/WGacCallbackService.h"
#include "Services/WGacClipboardService.h"
#include "Services/WGacDialogService.h"
#include "Services/WGacImageService.h"
#include "Services/WGacInputService.h"
#include "Services/WGacResourceService.h"
#include "Services/WGacScreenService.h"
#include "Wayland/WaylandDisplay.h"

namespace vl {
namespace presentation {
namespace wayland {

using namespace collections;

void GlobalTimerFunc();

class WGacController : public Object, public virtual INativeController, public virtual INativeWindowService
{
protected:
    List<WGacNativeWindow*> windows;
    INativeWindow* mainWindow;

    WGacCallbackService callbackService;
    WGacInputService inputService;
    WGacResourceService resourceService;
    WGacScreenService screenService;
    WGacAsyncService asyncService;
    WGacClipboardService clipboardService;
    WGacImageService imageService;
    WGacDialogService dialogService;

    WaylandDisplay* display;
    bool running;

public:
    WGacController()
        : mainWindow(nullptr)
        , inputService(&GlobalTimerFunc)
        , display(nullptr)
        , running(false)
    {
        display = new WaylandDisplay();
        if (!display->Connect()) {
            delete display;
            display = nullptr;
        }
        SetWaylandDisplay(display);
        clipboardService.Initialize();
        screenService.RefreshScreenInformation();
    }

    ~WGacController()
    {
        inputService.StopTimer();
        // Cleanup clipboard before disconnecting display
        clipboardService.Cleanup();
        if (display) {
            display->Disconnect();
            delete display;
        }
    }

    WaylandDisplay* GetDisplay() { return display; }

    void InvokeGlobalTimer()
    {
        asyncService.ExecuteAsyncTasks();
        callbackService.InvokeGlobalTimer();
    }

    //========================================[INativeWindowService]========================================

    const NativeWindowFrameConfig& GetMainWindowFrameConfig() override
    {
        return NativeWindowFrameConfig::Default;
    }

    const NativeWindowFrameConfig& GetNonMainWindowFrameConfig() override
    {
        static const NativeWindowFrameConfig config = {
            .MaximizedBoxOption = BoolOption::AlwaysFalse,
            .MinimizedBoxOption = BoolOption::AlwaysFalse,
            .CustomFrameEnabled = BoolOption::AlwaysTrue,
        };
        return config;
    }

    INativeWindow* CreateNativeWindow(INativeWindow::WindowMode mode) override
    {
        WGacNativeWindow* window = new WGacNativeWindow(display, mode);
        if (!window->Create()) {
            delete window;
            return nullptr;
        }
        callbackService.InvokeNativeWindowCreated(window);
        windows.Add(window);
        return window;
    }

    void DestroyNativeWindow(INativeWindow* _window) override
    {
        WGacNativeWindow* window = dynamic_cast<WGacNativeWindow*>(_window);
        if (window && windows.Contains(window))
        {
            callbackService.InvokeNativeWindowDestroying(window);
            windows.Remove(window);
            delete window;
        }
    }

    INativeWindow* GetMainWindow() override
    {
        return mainWindow;
    }

    void Run(INativeWindow* window) override
    {
        mainWindow = window;
        running = true;

        inputService.StartTimer();

        // Show the main window - GacUI expects the window to be shown when Run() returns
        window->Show();

        // Wait for the window to be configured
        auto* wgacWindow = dynamic_cast<WGacNativeWindow*>(window);
        while (running && wgacWindow && !wgacWindow->IsVisible()) {
            if (display) {
                display->Dispatch();
            }
        }

        while (running) {
            if (display) {
                display->DispatchPending();

                // Check if main window is closed
                auto* wgacWindow = dynamic_cast<WGacNativeWindow*>(mainWindow);
                if (wgacWindow && !wgacWindow->IsVisible()) {
                    break;
                }

                // Process timer
                InvokeGlobalTimer();

                // Wait for events with timeout
                display->Dispatch();
            }
        }

        inputService.StopTimer();
    }

    bool RunOneCycle() override
    {
        return true;
    }

    INativeWindow* GetWindow(NativePoint location) override
    {
        WGacNativeWindow* result = nullptr;
        for (vint i = 0; i < windows.Count(); i++)
        {
            WGacNativeWindow* window = windows[i];
            NativeRect rect = window->GetClientBoundsInScreen();
            if (rect.Contains(location))
            {
                if (!result)
                {
                    result = window;
                }
            }
        }
        return result;
    }

    //========================================[INativeController]========================================

    INativeCallbackService* CallbackService() override
    {
        return &callbackService;
    }

    INativeResourceService* ResourceService() override
    {
        return &resourceService;
    }

    INativeAsyncService* AsyncService() override
    {
        return &asyncService;
    }

    INativeClipboardService* ClipboardService() override
    {
        return &clipboardService;
    }

    INativeImageService* ImageService() override
    {
        return &imageService;
    }

    INativeScreenService* ScreenService() override
    {
        return &screenService;
    }

    INativeInputService* InputService() override
    {
        return &inputService;
    }

    INativeDialogService* DialogService() override
    {
        return &dialogService;
    }

    INativeWindowService* WindowService() override
    {
        return this;
    }

    WString GetExecutablePath() override
    {
        char path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (len != -1) {
            path[len] = '\0';
            return atow(AString(path));
        }
        return L"";
    }
};

//========================================[Global Functions]========================================

INativeController* wGacController = nullptr;

INativeController* GetWGacController()
{
    if (!wGacController) {
        wGacController = new WGacController();
    }
    return wGacController;
}

void DestroyWGacController(INativeController* controller)
{
    delete controller;
    wGacController = nullptr;
}

void GlobalTimerFunc()
{
    dynamic_cast<WGacController*>(GetWGacController())->InvokeGlobalTimer();
}

}
}
}
