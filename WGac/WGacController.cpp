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
#include <stdexcept>

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
    AString applicationId;

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
    bool finalizing;

public:
    WGacController()
        : mainWindow(nullptr)
        , applicationId(AString::Unmanaged("gacui"))
        , inputService(&GlobalTimerFunc)
        , display(nullptr)
        , running(false)
        , finalizing(false)
    {
        display = new WaylandDisplay();
        if (!display->Connect() || !display->GetLibdecorContext())
        {
            auto message = display->GetLastError();
            delete display;
            display = nullptr;
            SetWaylandDisplay(nullptr);
            throw std::runtime_error(
                message.empty()
                    ? "wGac initialization failed: the Wayland display or libdecor is unavailable."
                    : "wGac initialization failed: " + message);
        }
        SetWaylandDisplay(display);
        clipboardService.Initialize();
        screenService.RefreshScreenInformation();
    }

    ~WGacController()
    {
        finalizing = true;
        inputService.StopTimer();

        while (windows.Count() > 0)
        {
            DestroyNativeWindow(windows[windows.Count() - 1]);
        }

        // Cleanup clipboard before disconnecting display
        clipboardService.Cleanup();
        if (display)
        {
            display->Disconnect();
        }
        SetWaylandDisplay(nullptr);
        delete display;
        display = nullptr;
    }

    WaylandDisplay* GetDisplay() { return display; }

    void GetAllCreatedWGacNativeWindows(List<WGacNativeWindow*>& result)
    {
        for (vint i = 0; i < windows.Count(); i++)
        {
            result.Add(windows[i]);
        }
    }

    const AString& GetApplicationId()
    {
        return applicationId;
    }

    void NotifyNativeWindowTitleChanged(WGacNativeWindow* window)
    {
        if (!window || mainWindow != window)
        {
            return;
        }

        auto newApplicationId = wtoa(window->GetTitle());
        if (newApplicationId.Length() == 0)
        {
            newApplicationId = AString::Unmanaged("gacui");
        }
        if (applicationId == newApplicationId)
        {
            return;
        }

        // GNOME Shell uses an unmatched Wayland app ID as the dock label.
        // Keep every toplevel grouped under the main window's current title.
        applicationId = newApplicationId;
        for (vint i = 0; i < windows.Count(); i++)
        {
            windows[i]->SetApplicationId(applicationId);
        }
    }

    void InvokeGlobalTimer()
    {
        asyncService.ExecuteAsyncTasks();
        callbackService.InvokeGlobalTimer();
    }

    void StopRunning()
    {
        inputService.StopTimer();
        running = false;
        asyncService.Stop();
    }

    //========================================[INativeWindowService]========================================

    const NativeWindowFrameConfig& GetMainWindowFrameConfig() override
    {
        return NativeWindowFrameConfig::Default;
    }

    const NativeWindowFrameConfig& GetNonMainWindowFrameConfig() override
    {
        return NativeWindowFrameConfig::Default;
    }

    INativeWindow* CreateNativeWindow(INativeWindow::WindowMode mode) override
    {
        WGacNativeWindow* window = new WGacNativeWindow(display, mode);
        if (!window->Create()) {
            auto message = display
                ? display->GetLastError()
                : std::string();
            delete window;
            throw std::runtime_error(
                message.empty()
                    ? "wGac failed to create a native Wayland window."
                    : "wGac failed to create a native Wayland window: " +
                        message);
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
            // GuiApplication is already gone when the controller destructor
            // cleans up dormant popup windows.
            if (!finalizing)
            {
                List<INativeWindowListener*> copiedListeners;
                CopyFrom(copiedListeners, window->listeners);
                for (auto listener : copiedListeners)
                {
                    if (window->listeners.Contains(listener))
                    {
                        listener->Destroying();
                    }
                }
            }
            callbackService.InvokeNativeWindowDestroying(window);
            windows.Remove(window);
            if (mainWindow == window)
            {
                mainWindow = nullptr;
            }
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
        NotifyNativeWindowTitleChanged(
            dynamic_cast<WGacNativeWindow*>(mainWindow));
        running = true;

        inputService.StartTimer();

        try
        {
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
                InvokeGlobalTimer();

                if (display && display->DispatchWithTimeout(16) < 0) {
                    break;
                }

                // Check if main window is closed
                auto* wgacWindow = dynamic_cast<WGacNativeWindow*>(mainWindow);
                if (!wgacWindow || !wgacWindow->IsVisible()) {
                    break;
                }
            }
        }
        catch (...)
        {
            StopRunning();
            throw;
        }
        StopRunning();
    }

    bool RunOneCycle() override
    {
        try
        {
            InvokeGlobalTimer();
            if (display && display->DispatchWithTimeout(16) < 0)
            {
                if (running)
                {
                    StopRunning();
                }
                return false;
            }
            auto* wgacWindow = dynamic_cast<WGacNativeWindow*>(mainWindow);
            auto continueRunning = running && wgacWindow && wgacWindow->IsVisible();
            if (!continueRunning && running)
            {
                StopRunning();
            }
            return continueRunning;
        }
        catch (...)
        {
            if (running)
            {
                StopRunning();
            }
            throw;
        }
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
        // GuiInitializeUtilities installs FakeDialogService when the platform
        // controller does not provide a dialog service.
        return nullptr;
    }

    INativeAutomationService* AutomationService() override
    {
        // GuiInitializeUtilities substitutes
        // INativeAutomationService::UnavailableService().
        return nullptr;
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

void GetAllCreatedWGacNativeWindows(List<WGacNativeWindow*>& windows)
{
    dynamic_cast<WGacController*>(GetWGacController())->GetAllCreatedWGacNativeWindows(windows);
}

AString GetWGacApplicationId()
{
    auto controller = dynamic_cast<WGacController*>(wGacController);
    return controller
        ? controller->GetApplicationId()
        : AString::Unmanaged("gacui");
}

void NotifyWGacNativeWindowTitleChanged(WGacNativeWindow* window)
{
    auto controller = dynamic_cast<WGacController*>(wGacController);
    if (controller)
    {
        controller->NotifyNativeWindowTitleChanged(window);
    }
}

void GlobalTimerFunc()
{
    dynamic_cast<WGacController*>(GetWGacController())->InvokeGlobalTimer();
}

}
}
}
