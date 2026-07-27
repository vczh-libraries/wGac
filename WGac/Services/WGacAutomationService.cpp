#include "WGacAutomationService.h"
#include "../WGacNativeWindow.h"

namespace vl
{
	namespace presentation
	{
		namespace wayland
		{

/***********************************************************************
WGacAutomationServiceBase
***********************************************************************/

			template<typename TBase>
			WString WGacAutomationServiceBase<TBase>::RunIOCommandInternal(Nullable<WString> windowId, const WString& ioCommand)
			{
				auto wgacWindow = dynamic_cast<WGacNativeWindow*>(this->GetNativeWindow(windowId));
				if (!wgacWindow)
				{
					return L"!Invalid window.";
				}

				return RunIOCommandOnNativeWindow(&this->ioCommandState, GetWGacController(), wgacWindow, wgacWindow->listeners, ioCommand);
			}

			template<typename TBase>
			void WGacAutomationServiceBase<TBase>::Stop()
			{
				TBase::Stop();
			}

			template<typename TBase>
			INativeAutomationService::IOCommandAvailability WGacAutomationServiceBase<TBase>::CanRunIOCommands()
			{
				return INativeAutomationService::IOCommandAvailability::Enabled;
			}

/***********************************************************************
WGacAutomationService
***********************************************************************/

			Nullable<WString> WGacAutomationService::GetNativeWindowId(INativeWindow* window)
			{
#define ERROR_MESSAGE_PREFIX L"vl::presentation::wayland::WGacAutomationService::GetNativeWindowId(INativeWindow*)#"
				auto wgacWindow = dynamic_cast<WGacNativeWindow*>(window);
				collections::List<WGacNativeWindow*> windows;
				GetAllCreatedWGacNativeWindows(windows);
				CHECK_ERROR(windows.Contains(wgacWindow), ERROR_MESSAGE_PREFIX L"The specified INativeWindow instance should be native.");
				return utow(static_cast<vuint>(reinterpret_cast<intptr_t>(wgacWindow)));
#undef ERROR_MESSAGE_PREFIX
			}

			INativeWindow* WGacAutomationService::GetNativeWindow(Nullable<WString> windowId)
			{
				if (windowId)
				{
					auto wgacWindow = reinterpret_cast<WGacNativeWindow*>(static_cast<intptr_t>(wtou64(windowId.Value())));
					collections::List<WGacNativeWindow*> windows;
					GetAllCreatedWGacNativeWindows(windows);
					return windows.Contains(wgacWindow) ? wgacWindow : nullptr;
				}
				else
				{
					return GetWGacController()->WindowService()->GetMainWindow();
				}
			}

			WGacAutomationService::WGacAutomationService()
			{
			}

			WGacAutomationService::~WGacAutomationService()
			{
			}

/***********************************************************************
WGacAutomationServiceHosted
***********************************************************************/

			WGacAutomationServiceHosted::WGacAutomationServiceHosted()
			{
			}

			WGacAutomationServiceHosted::~WGacAutomationServiceHosted()
			{
			}

/***********************************************************************
WGacAutomationServiceRenderer
***********************************************************************/

			WGacAutomationServiceRenderer::WGacAutomationServiceRenderer(remote_renderer::GuiRemoteRendererSingle* renderer)
				: WGacAutomationServiceBase<AutomationServiceRenderer>(renderer)
			{
			}

			WGacAutomationServiceRenderer::~WGacAutomationServiceRenderer()
			{
			}

			INativeAutomationService::IOCommandAvailability WGacAutomationServiceRenderer::CanRunIOCommands()
			{
				return AutomationServiceRenderer::CanRunIOCommands();
			}
		}
	}
}
